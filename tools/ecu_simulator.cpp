#include <iostream>
#include <fmt/core.h>
#include <thread>
#include <chrono>
#include <random>
#include <map>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sstream>
#include <vector>

/**
 * @brief ECU Simulator - Generates test ECU data for vehicle systems
 * 
 * This simulator generates realistic ECU data from various vehicle systems:
 * - Engine ECU (RPM, temperature, pressure, throttle position)
 * - Transmission ECU (gear, speed, temperature)
 * - Brake ECU (brake pressure, ABS status)
 * - Battery ECU (voltage, current, temperature, state of charge)
 */

class ECUSimulator {
public:
    ECUSimulator(const std::string& host, int port) 
        : host_(host), port_(port), running_(false) {
        // Initialize random number generators for each ECU
        engine_rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count());
        transmission_rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count() + 1000);
        brake_rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count() + 2000);
        battery_rng_.seed(std::chrono::steady_clock::now().time_since_epoch().count() + 3000);
    }

    bool connect() {
        sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_ < 0) {
            fmt::print(stderr, "Error creating socket\n");
            return false;
        }

        struct sockaddr_in server_addr;
        std::memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        
        if (inet_pton(AF_INET, host_.c_str(), &server_addr.sin_addr) <= 0) {
            fmt::print(stderr, "Invalid address/Address not supported\n");
            close(sockfd_);
            return false;
        }

        if (::connect(sockfd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            fmt::print(stderr, "Connection failed to {}:{}\n", host_, port_);
            close(sockfd_);
            return false;
        }

        fmt::print("Connected to gateway at {}:{}\n", host_, port_);
        return true;
    }

    void disconnect() {
        if (sockfd_ >= 0) {
            close(sockfd_);
            sockfd_ = -1;
        }
    }

    bool sendData(const std::string& jsonStr) {
        if (sockfd_ < 0) return false;
        std::string data = jsonStr + "\n";
        ssize_t sent = ::send(sockfd_, data.c_str(), data.length(), 0);
        return sent > 0;
    }

    std::string generateEngineData(int sequence) {
        std::uniform_real_distribution<double> rpmDist(800, 6000);
        std::uniform_real_distribution<double> tempDist(75, 105);
        std::uniform_real_distribution<double> pressureDist(0.8, 1.5);
        std::uniform_real_distribution<double> throttleDist(0, 100);

        int rpm = static_cast<int>(rpmDist(engine_rng_));
        double temp = tempDist(engine_rng_);
        double pressure = pressureDist(engine_rng_);
        double throttle = throttleDist(engine_rng_);

        return fmt::format(
            R"({{"id":"engine-{:06d}","ecuId":"engine","data":{{"rpm":"{}","temperature":"{:.1f}","pressure":"{:.2f}","throttle_position":"{:.1f}"}}}})",
            sequence, rpm, temp, pressure, throttle
        );
    }

    std::string generateTransmissionData(int sequence) {
        std::uniform_int_distribution<int> gearDist(0, 6);
        std::uniform_real_distribution<double> speedDist(0, 120);
        std::uniform_real_distribution<double> tempDist(60, 90);

        int gear = gearDist(transmission_rng_);
        double speed = speedDist(transmission_rng_);
        double temp = tempDist(transmission_rng_);

        return fmt::format(
            R"({{"id":"transmission-{:06d}","ecuId":"transmission","data":{{"gear":"{}","speed":"{:.1f}","temperature":"{:.1f}"}}}})",
            sequence, gear, speed, temp
        );
    }

    std::string generateBrakeData(int sequence) {
        std::uniform_real_distribution<double> pressureDist(0, 100);
        std::uniform_int_distribution<int> absDist(0, 1);

        double pressure = pressureDist(brake_rng_);
        bool absActive = absDist(brake_rng_) == 1;

        return fmt::format(
            R"({{"id":"brake-{:06d}","ecuId":"brake","data":{{"brake_pressure":"{:.1f}","abs_active":"{}"}}}})",
            sequence, pressure, absActive ? "true" : "false"
        );
    }

    std::string generateBatteryData(int sequence) {
        std::uniform_real_distribution<double> voltageDist(11.5, 14.5);
        std::uniform_real_distribution<double> currentDist(-50, 50);
        std::uniform_real_distribution<double> tempDist(15, 35);
        std::uniform_real_distribution<double> socDist(20, 100);

        double voltage = voltageDist(battery_rng_);
        double current = currentDist(battery_rng_);
        double temp = tempDist(battery_rng_);
        double soc = socDist(battery_rng_);

        return fmt::format(
            R"({{"id":"battery-{:06d}","ecuId":"battery","data":{{"voltage":"{:.2f}","current":"{:.2f}","temperature":"{:.1f}","state_of_charge":"{:.1f}"}}}})",
            sequence, voltage, current, temp, soc
        );
    }

    void run(int durationSeconds = 60, int intervalMs = 1000) {
        running_ = true;
        int sequence = 0;
        auto startTime = std::chrono::steady_clock::now();

        fmt::print("Starting ECU simulator...\n");
        fmt::print("  Duration: {} seconds\n", durationSeconds);
        fmt::print("  Interval: {} ms\n", intervalMs);
        fmt::print("  ECUs: engine, transmission, brake, battery\n\n");

        while (running_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            
            if (elapsed >= durationSeconds) {
                break;
            }

            // Generate data from all ECUs
            std::vector<std::pair<std::string, std::string>> ecuData = {
                {"engine", generateEngineData(sequence)},
                {"transmission", generateTransmissionData(sequence)},
                {"brake", generateBrakeData(sequence)},
                {"battery", generateBatteryData(sequence)}
            };

            // Send each ECU's data
            for (const auto& [ecuId, data] : ecuData) {
                if (sendData(data)) {
                    fmt::print("[SENT] {}: sequence {}\n", ecuId, sequence);
                } else {
                    fmt::print(stderr, "[ERROR] Failed to send data from {}\n", ecuId);
                }
            }

            sequence++;
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }

        fmt::print("\nECU simulator stopped. Sent {} sequences.\n", sequence);
    }

    void stop() {
        running_ = false;
    }

private:
    std::string host_;
    int port_;
    int sockfd_ = -1;
    bool running_;
    
    // Random number generators for each ECU
    std::mt19937 engine_rng_;
    std::mt19937 transmission_rng_;
    std::mt19937 brake_rng_;
    std::mt19937 battery_rng_;
};

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    int duration = 60;
    int interval = 1000;

    // Parse command line arguments
    if (argc > 1) host = argv[1];
    if (argc > 2) port = std::stoi(argv[2]);
    if (argc > 3) duration = std::stoi(argv[3]);
    if (argc > 4) interval = std::stoi(argv[4]);

    fmt::print("╔══════════════════════════════════════════════════════════╗\n");
    fmt::print("║              ECU Data Simulator                          ║\n");
    fmt::print("╚══════════════════════════════════════════════════════════╝\n\n");

    ECUSimulator simulator(host, port);

    if (!simulator.connect()) {
        fmt::print(stderr, "Failed to connect to gateway. Make sure the gateway is running.\n");
        return 1;
    }

    simulator.run(duration, interval);
    simulator.disconnect();

    return 0;
}

