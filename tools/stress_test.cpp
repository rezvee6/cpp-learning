#include <iostream>
#include <fmt/core.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <random>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <mutex>

/**
 * @brief Stress Test Tool for ECU Gateway
 * 
 * This tool stress tests the ECU gateway by:
 * 1. Creating multiple concurrent ECU simulator connections
 * 2. Sending high-frequency messages
 * 3. Testing REST API endpoints under load
 * 4. Monitoring performance metrics
 */

class StressTest {
public:
    StressTest(const std::string& host, int tcpPort, int httpPort)
        : host_(host), tcpPort_(tcpPort), httpPort_(httpPort),
          messagesSent_(0), messagesFailed_(0), apiRequests_(0), apiFailures_(0) {}

    bool httpRequest(const std::string& path, int& statusCode) {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            apiFailures_++;
            return false;
        }

        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(httpPort_);
        inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

        if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sockfd);
            apiFailures_++;
            return false;
        }

        // Send HTTP GET request
        std::string request = "GET " + path + " HTTP/1.1\r\n"
                           + "Host: " + host_ + "\r\n"
                           + "Connection: close\r\n"
                           + "\r\n";

        if (::send(sockfd, request.c_str(), request.length(), 0) < 0) {
            close(sockfd);
            apiFailures_++;
            return false;
        }

        // Read response (just first line for status code)
        char buffer[1024];
        ssize_t bytes_read = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
        close(sockfd);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string response(buffer);
            // Parse status code from "HTTP/1.1 200 OK"
            size_t space1 = response.find(' ');
            size_t space2 = response.find(' ', space1 + 1);
            if (space1 != std::string::npos && space2 != std::string::npos) {
                statusCode = std::stoi(response.substr(space1 + 1, space2 - space1 - 1));
                apiRequests_++;
                return statusCode == 200;
            }
        }

        apiFailures_++;
        return false;
    }

    void httpLoadTest(int durationSeconds, int requestsPerSecond) {
        fmt::print("[HTTP] Starting HTTP load test: {} req/s for {}s\n", 
                   requestsPerSecond, durationSeconds);
        
        auto startTime = std::chrono::steady_clock::now();
        int requestCount = 0;
        int intervalMs = 1000 / requestsPerSecond;

        std::vector<std::thread> threads;
        std::vector<std::string> endpoints = {
            "/health",
            "/api/ecus",
            "/api/data",
            "/api/ecus/engine",
            "/api/ecus/transmission",
            "/api/ecus/brake",
            "/api/ecus/battery"
        };

        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            if (elapsed >= durationSeconds) break;

            // Spawn threads for concurrent requests
            for (const auto& endpoint : endpoints) {
                threads.emplace_back([this, endpoint]() {
                    int statusCode;
                    httpRequest(endpoint, statusCode);
                });
            }

            // Limit thread count
            if (threads.size() > 100) {
                for (auto& t : threads) {
                    if (t.joinable()) t.join();
                }
                threads.clear();
            }

            requestCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }

        // Wait for remaining threads
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        fmt::print("[HTTP] Load test complete: {} requests, {} failures\n", 
                   apiRequests_.load(), apiFailures_.load());
    }

    void tcpLoadTest(int numConnections, int messagesPerConnection, int intervalMs) {
        fmt::print("[TCP] Starting TCP load test: {} connections, {} msgs each\n", 
                   numConnections, messagesPerConnection);
        
        std::vector<std::thread> threads;
        std::atomic<int> activeConnections{0};

        for (int i = 0; i < numConnections; ++i) {
            threads.emplace_back([this, i, messagesPerConnection, intervalMs, &activeConnections]() {
                int sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if (sockfd < 0) {
                    messagesFailed_++;
                    return;
                }

                struct sockaddr_in server_addr;
                std::memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_family = AF_INET;
                server_addr.sin_port = htons(tcpPort_);
                inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr);

                if (::connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                    close(sockfd);
                    messagesFailed_++;
                    return;
                }

                activeConnections++;
                std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count() + i);
                std::uniform_real_distribution<double> valueDist(0, 100);

                for (int j = 0; j < messagesPerConnection; ++j) {
                    std::string ecuId = (j % 4 == 0) ? "engine" : 
                                       (j % 4 == 1) ? "transmission" :
                                       (j % 4 == 2) ? "brake" : "battery";
                    
                    std::string json = fmt::format(
                        R"({{"id":"stress-{:06d}-{:06d}","ecuId":"{}","data":{{"value":"{:.2f}"}}}})",
                        i, j, ecuId, valueDist(rng)
                    );
                    
                    std::string data = json + "\n";
                    ssize_t sent = ::send(sockfd, data.c_str(), data.length(), 0);
                    
                    if (sent > 0) {
                        messagesSent_++;
                    } else {
                        messagesFailed_++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
                }

                close(sockfd);
                activeConnections--;
            });
        }

        // Monitor progress
        auto startTime = std::chrono::steady_clock::now();
        while (activeConnections.load() > 0) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - startTime).count();
            fmt::print("[TCP] {}s: {} active connections, {} sent, {} failed\n",
                      elapsed, activeConnections.load(), messagesSent_.load(), messagesFailed_.load());
        }

        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }

        fmt::print("[TCP] Load test complete: {} sent, {} failed\n", 
                   messagesSent_.load(), messagesFailed_.load());
    }

    void runStressTest(int numConnections, int messagesPerConnection, 
                      int tcpIntervalMs, int httpDuration, int httpRps) {
        fmt::print("╔══════════════════════════════════════════════════════════╗\n");
        fmt::print("║           ECU Gateway Stress Test                        ║\n");
        fmt::print("╚══════════════════════════════════════════════════════════╝\n\n");

        fmt::print("Configuration:\n");
        fmt::print("  TCP: {} connections, {} msgs each, {}ms interval\n", 
                   numConnections, messagesPerConnection, tcpIntervalMs);
        fmt::print("  HTTP: {}s duration, {} req/s\n\n", httpDuration, httpRps);

        // Run TCP and HTTP tests concurrently
        std::thread tcpThread([this, numConnections, messagesPerConnection, tcpIntervalMs]() {
            tcpLoadTest(numConnections, messagesPerConnection, tcpIntervalMs);
        });

        std::thread httpThread([this, httpDuration, httpRps]() {
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait for some data
            httpLoadTest(httpDuration, httpRps);
        });

        tcpThread.join();
        httpThread.join();

        // Print summary
        fmt::print("\n╔══════════════════════════════════════════════════════════╗\n");
        fmt::print("║                    Test Summary                          ║\n");
        fmt::print("╚══════════════════════════════════════════════════════════╝\n");
        fmt::print("TCP Messages:\n");
        fmt::print("  Sent:    {}\n", messagesSent_.load());
        fmt::print("  Failed:  {}\n", messagesFailed_.load());
        fmt::print("  Success: {:.2f}%\n", 
                   messagesSent_.load() > 0 ? 
                   100.0 * (messagesSent_.load() - messagesFailed_.load()) / messagesSent_.load() : 0.0);
        fmt::print("\nHTTP Requests:\n");
        fmt::print("  Total:   {}\n", apiRequests_.load());
        fmt::print("  Failed:  {}\n", apiFailures_.load());
        fmt::print("  Success: {:.2f}%\n",
                   apiRequests_.load() > 0 ?
                   100.0 * (apiRequests_.load() - apiFailures_.load()) / apiRequests_.load() : 0.0);
    }

private:
    std::string host_;
    int tcpPort_;
    int httpPort_;
    std::atomic<int> messagesSent_;
    std::atomic<int> messagesFailed_;
    std::atomic<int> apiRequests_;
    std::atomic<int> apiFailures_;
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int tcpPort = 8080;
    int httpPort = 8081;
    int numConnections = 10;
    int messagesPerConnection = 100;
    int tcpIntervalMs = 10;  // 10ms = 100 msg/s per connection
    int httpDuration = 30;
    int httpRps = 50;

    // Parse arguments
    if (argc > 1) numConnections = std::stoi(argv[1]);
    if (argc > 2) messagesPerConnection = std::stoi(argv[2]);
    if (argc > 3) tcpIntervalMs = std::stoi(argv[3]);
    if (argc > 4) httpDuration = std::stoi(argv[4]);
    if (argc > 5) httpRps = std::stoi(argv[5]);

    StressTest test(host, tcpPort, httpPort);
    test.runStressTest(numConnections, messagesPerConnection, tcpIntervalMs, httpDuration, httpRps);

    return 0;
}

