#include <iostream>
#include <fmt/core.h>
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/MessageTypes.h"
#include "include/StateMachine.h"
#include "include/ExampleStates.h"
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <map>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>

/**
 * @brief ECU Gateway - Receives ECU data and exposes REST API
 * 
 * This gateway:
 * 1. Receives ECU data from simulators via TCP socket
 * 2. Processes and stores ECU data in memory
 * 3. Exposes REST API endpoints for clients to consume data
 */

// Data storage for ECU data
using ECUDataMap = std::map<std::string, std::map<std::string, std::string>>;

struct ECUDataStore {
    std::mutex mutex;
    ECUDataMap latestData; // ecuId -> {param -> value}
    std::map<std::string, std::chrono::system_clock::time_point> timestamps; // ecuId -> timestamp
    std::vector<std::shared_ptr<ECUDataMessage>> history; // Store recent history
    static constexpr size_t MAX_HISTORY = 1000;
    
    void update(const std::string& ecuId, const std::map<std::string, std::string>& data) {
        std::lock_guard<std::mutex> lock(mutex);
        latestData[ecuId] = data;
        timestamps[ecuId] = std::chrono::system_clock::now();
    }
    
    ECUDataMap getAllLatest() {
        std::lock_guard<std::mutex> lock(mutex);
        return latestData;
    }
    
    std::map<std::string, std::string> getECUData(const std::string& ecuId) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = latestData.find(ecuId);
        if (it != latestData.end()) {
            return it->second;
        }
        return {};
    }
    
    std::vector<std::string> getECUIds() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> ids;
        for (const auto& [id, _] : latestData) {
            ids.push_back(id);
        }
        return ids;
    }
};

ECUDataStore dataStore;

// Simple JSON parser helper - handles nested structure
std::map<std::string, std::string> parseJSONData(const std::string& jsonStr) {
    std::map<std::string, std::string> result;
    // Find the data section
    size_t pos = jsonStr.find("\"data\":{");
    if (pos == std::string::npos) return result;
    
    pos += 8; // Skip "data":{
    
    // Find matching closing brace (handle nested braces)
    int braceCount = 1;
    size_t end = pos;
    while (end < jsonStr.length() && braceCount > 0) {
        if (jsonStr[end] == '{') braceCount++;
        else if (jsonStr[end] == '}') braceCount--;
        end++;
    }
    if (braceCount != 0) return result;
    end--; // Back up to the closing brace
    
    std::string dataSection = jsonStr.substr(pos, end - pos);
    
    // Parse nested structure: "ParameterName":{"value":X,"unit":"Y","status":"Z","timestamp":"T"}
    size_t paramStart = 0;
    while ((paramStart = dataSection.find("\"", paramStart)) != std::string::npos) {
        size_t paramEnd = dataSection.find("\"", paramStart + 1);
        if (paramEnd == std::string::npos) break;
        std::string paramName = dataSection.substr(paramStart + 1, paramEnd - paramStart - 1);
        
        // Find the value field
        size_t valuePos = dataSection.find("\"value\":", paramEnd);
        if (valuePos == std::string::npos) {
            paramStart = paramEnd + 1;
            continue;
        }
        
        valuePos += 8; // Skip "value":
        // Skip whitespace
        while (valuePos < dataSection.length() && (dataSection[valuePos] == ' ' || dataSection[valuePos] == '\t')) {
            valuePos++;
        }
        
        std::string value;
        if (valuePos < dataSection.length() && dataSection[valuePos] == '"') {
            // String value
            valuePos++;
            size_t valueEnd = dataSection.find("\"", valuePos);
            if (valueEnd != std::string::npos) {
                value = dataSection.substr(valuePos, valueEnd - valuePos);
            }
        } else {
            // Numeric value
            size_t valueEnd = valuePos;
            while (valueEnd < dataSection.length() && 
                   (std::isdigit(dataSection[valueEnd]) || dataSection[valueEnd] == '.' || 
                    dataSection[valueEnd] == '-' || dataSection[valueEnd] == 'e' || 
                    dataSection[valueEnd] == 'E' || dataSection[valueEnd] == '+')) {
                valueEnd++;
            }
            if (valueEnd > valuePos) {
                value = dataSection.substr(valuePos, valueEnd - valuePos);
            }
        }
        
        // Store flattened: paramName.value, paramName.unit, paramName.status, paramName.timestamp
        if (!value.empty()) {
            result[paramName + ".value"] = value;
        }
        
        // Extract unit
        size_t unitPos = dataSection.find("\"unit\":\"", paramEnd);
        if (unitPos != std::string::npos) {
            unitPos += 8;
            size_t unitEnd = dataSection.find("\"", unitPos);
            if (unitEnd != std::string::npos) {
                result[paramName + ".unit"] = dataSection.substr(unitPos, unitEnd - unitPos);
            }
        }
        
        // Extract status
        size_t statusPos = dataSection.find("\"status\":\"", paramEnd);
        if (statusPos != std::string::npos) {
            statusPos += 10;
            size_t statusEnd = dataSection.find("\"", statusPos);
            if (statusEnd != std::string::npos) {
                result[paramName + ".status"] = dataSection.substr(statusPos, statusEnd - statusPos);
            }
        }
        
        // Extract timestamp
        size_t tsPos = dataSection.find("\"timestamp\":\"", paramEnd);
        if (tsPos != std::string::npos) {
            tsPos += 13;
            size_t tsEnd = dataSection.find("\"", tsPos);
            if (tsEnd != std::string::npos) {
                result[paramName + ".timestamp"] = dataSection.substr(tsPos, tsEnd - tsPos);
            }
        }
        
        // Move to next parameter (find next parameter start)
        size_t nextParam = dataSection.find("\"", paramEnd + 1);
        if (nextParam == std::string::npos) break;
        paramStart = nextParam;
    }
    
    return result;
}

std::string extractJSONField(const std::string& jsonStr, const std::string& field) {
    std::string search = "\"" + field + "\":\"";
    size_t pos = jsonStr.find(search);
    if (pos == std::string::npos) return "";
    pos += search.length();
    size_t end = jsonStr.find("\"", pos);
    if (end == std::string::npos) return "";
    return jsonStr.substr(pos, end - pos);
}

// TCP server for receiving ECU data
class TCPServer {
public:
    TCPServer(int port) : port_(port), running_(false) {}
    
    void start() {
        running_ = true;
        serverThread_ = std::thread([this]() { this->run(); });
    }
    
    void stop() {
        running_ = false;
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }
    
private:
    void run() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            fmt::print(stderr, "Error creating TCP server socket\n");
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            fmt::print(stderr, "Error binding TCP server socket to port {}\n", port_);
            close(server_fd);
            return;
        }
        
        if (listen(server_fd, 10) < 0) {
            fmt::print(stderr, "Error listening on TCP server socket\n");
            close(server_fd);
            return;
        }
        
        fmt::print("✓ TCP server listening on port {} for ECU data\n", port_);
        
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd < 0) {
                if (running_) {
                    fmt::print(stderr, "Error accepting connection\n");
                }
                continue;
            }
            
            // Handle client in a separate thread
            std::thread([this, client_fd]() {
                this->handleClient(client_fd);
            }).detach();
        }
        
        close(server_fd);
    }
    
    void handleClient(int client_fd) {
        char buffer[4096];
        std::string bufferStr;
        
        while (running_) {
            ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
            if (bytes_read <= 0) {
                break;
            }
            
            buffer[bytes_read] = '\0';
            bufferStr += buffer;
            
            // Process complete JSON messages (newline-separated)
            size_t pos = 0;
            while ((pos = bufferStr.find('\n')) != std::string::npos) {
                std::string jsonMsg = bufferStr.substr(0, pos);
                bufferStr = bufferStr.substr(pos + 1);
                
                if (!jsonMsg.empty()) {
                    processECUData(jsonMsg);
                }
            }
        }
        
        close(client_fd);
    }
    
    void processECUData(const std::string& jsonStr) {
        std::string id = extractJSONField(jsonStr, "id");
        std::string ecuId = extractJSONField(jsonStr, "ecuId");
        
        if (ecuId.empty()) {
            fmt::print(stderr, "[ERROR] Invalid ECU data: missing ecuId\n");
            return;
        }
        
        std::map<std::string, std::string> data = parseJSONData(jsonStr);
        
        // Create ECUDataMessage and enqueue
        auto msg = std::make_shared<ECUDataMessage>(id, ecuId, data);
        
        // Update data store
        dataStore.update(ecuId, data);
        
        fmt::print("[GATEWAY] Received data from {} (ID: {})\n", ecuId, id);
    }
    
    int port_;
    std::atomic<bool> running_;
    std::thread serverThread_;
};

// Simple HTTP/REST server
class HTTPServer {
public:
    HTTPServer(int port) : port_(port), running_(false) {}
    
    void start() {
        running_ = true;
        serverThread_ = std::thread([this]() { this->run(); });
    }
    
    void stop() {
        running_ = false;
        if (serverThread_.joinable()) {
            serverThread_.join();
        }
    }
    
private:
    void run() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            fmt::print(stderr, "Error creating HTTP server socket\n");
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        struct sockaddr_in address;
        std::memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);
        
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            fmt::print(stderr, "Error binding HTTP server socket to port {}\n", port_);
            close(server_fd);
            return;
        }
        
        if (listen(server_fd, 10) < 0) {
            fmt::print(stderr, "Error listening on HTTP server socket\n");
            close(server_fd);
            return;
        }
        
        fmt::print("✓ HTTP server listening on port {} for REST API\n", port_);
        
        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            
            if (client_fd < 0) {
                if (running_) {
                    fmt::print(stderr, "Error accepting HTTP connection\n");
                }
                continue;
            }
            
            // Handle client
            handleHTTPClient(client_fd);
            close(client_fd);
        }
        
        close(server_fd);
    }
    
    void handleHTTPClient(int client_fd) {
        char buffer[4096];
        ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_read <= 0) return;
        
        buffer[bytes_read] = '\0';
        std::string request(buffer);
        
        // Parse request line
        size_t firstSpace = request.find(' ');
        size_t secondSpace = request.find(' ', firstSpace + 1);
        if (firstSpace == std::string::npos || secondSpace == std::string::npos) {
            sendError(client_fd, 400, "Bad Request");
            return;
        }
        
        std::string method = request.substr(0, firstSpace);
        std::string path = request.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        
        if (method == "GET") {
            handleGET(client_fd, path);
        } else {
            sendError(client_fd, 405, "Method Not Allowed");
        }
    }
    
    void handleGET(int client_fd, const std::string& path) {
        if (path == "/api/ecus" || path == "/api/ecus/") {
            // List all ECUs
            auto allData = dataStore.getAllLatest();
            std::string json = "{\"ecus\":[";
            bool first = true;
            for (const auto& [ecuId, _] : allData) {
                if (!first) json += ",";
                json += "\"" + ecuId + "\"";
                first = false;
            }
            json += "]}";
            sendJSON(client_fd, json);
        } else if (path.find("/api/ecus/") == 0) {
            // Get specific ECU data
            std::string ecuId = path.substr(10); // Skip "/api/ecus/"
            auto data = dataStore.getECUData(ecuId);
            if (data.empty()) {
                sendError(client_fd, 404, "ECU not found");
            } else {
                // Reconstruct nested structure from flattened data
                std::map<std::string, std::map<std::string, std::string>> nestedData;
                for (const auto& [key, value] : data) {
                    size_t dotPos = key.find('.');
                    if (dotPos != std::string::npos) {
                        std::string paramName = key.substr(0, dotPos);
                        std::string field = key.substr(dotPos + 1);
                        nestedData[paramName][field] = value;
                    }
                }
                
                std::string json = "{\"ecuId\":\"" + ecuId + "\",\"data\":{";
                bool first = true;
                for (const auto& [paramName, fields] : nestedData) {
                    if (!first) json += ",";
                    json += "\"" + paramName + "\":{";
                    bool firstField = true;
                    for (const auto& [field, val] : fields) {
                        if (!firstField) json += ",";
                        // Check if value is numeric (doesn't start with quote)
                        bool isNumeric = !val.empty() && (std::isdigit(val[0]) || val[0] == '-' || val[0] == '.');
                        if (isNumeric) {
                            json += "\"" + field + "\":" + val;
                        } else {
                            json += "\"" + field + "\":\"" + val + "\"";
                        }
                        firstField = false;
                    }
                    json += "}";
                    first = false;
                }
                json += "}}";
                sendJSON(client_fd, json);
            }
        } else if (path == "/api/data" || path == "/api/data/") {
            // Get all ECU data
            auto allData = dataStore.getAllLatest();
            std::string json = "{";
            bool firstEcu = true;
            for (const auto& [ecuId, data] : allData) {
                if (!firstEcu) json += ",";
                
                // Reconstruct nested structure from flattened data
                std::map<std::string, std::map<std::string, std::string>> nestedData;
                for (const auto& [key, value] : data) {
                    size_t dotPos = key.find('.');
                    if (dotPos != std::string::npos) {
                        std::string paramName = key.substr(0, dotPos);
                        std::string field = key.substr(dotPos + 1);
                        nestedData[paramName][field] = value;
                    }
                }
                
                json += "\"" + ecuId + "\":{";
                bool firstParam = true;
                for (const auto& [paramName, fields] : nestedData) {
                    if (!firstParam) json += ",";
                    json += "\"" + paramName + "\":{";
                    bool firstField = true;
                    for (const auto& [field, val] : fields) {
                        if (!firstField) json += ",";
                        bool isNumeric = !val.empty() && (std::isdigit(val[0]) || val[0] == '-' || val[0] == '.');
                        if (isNumeric) {
                            json += "\"" + field + "\":" + val;
                        } else {
                            json += "\"" + field + "\":\"" + val + "\"";
                        }
                        firstField = false;
                    }
                    json += "}";
                    firstParam = false;
                }
                json += "}";
                firstEcu = false;
            }
            json += "}";
            sendJSON(client_fd, json);
        } else if (path == "/" || path == "/health") {
            sendJSON(client_fd, "{\"status\":\"ok\",\"service\":\"ECU Gateway\"}");
        } else {
            sendError(client_fd, 404, "Not Found");
        }
    }
    
    void sendJSON(int client_fd, const std::string& json) {
        std::string response = "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Access-Control-Allow-Origin: *\r\n";
        response += "Content-Length: " + std::to_string(json.length()) + "\r\n";
        response += "\r\n";
        response += json;
        send(client_fd, response.c_str(), response.length(), 0);
    }
    
    void sendError(int client_fd, int code, const std::string& message) {
        std::string json = "{\"error\":" + std::to_string(code) + ",\"message\":\"" + message + "\"}";
        std::string response = "HTTP/1.1 " + std::to_string(code) + " " + message + "\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: " + std::to_string(json.length()) + "\r\n";
        response += "\r\n";
        response += json;
        send(client_fd, response.c_str(), response.length(), 0);
    }
    
    int port_;
    std::atomic<bool> running_;
    std::thread serverThread_;
};

int main() {
    fmt::print("╔══════════════════════════════════════════════════════════╗\n");
    fmt::print("║              ECU Data Gateway                            ║\n");
    fmt::print("║     Receives ECU data and exposes REST API               ║\n");
    fmt::print("╚══════════════════════════════════════════════════════════╝\n\n");
    
    const int TCP_PORT = 8080;  // Port for receiving ECU data
    const int HTTP_PORT = 8081; // Port for REST API
    
    // Create message queue and handler for processing
    MessageQueue messageQueue;
    MessageHandler messageHandler(messageQueue, 2);
    
    // Create TCP server for receiving ECU data
    TCPServer tcpServer(TCP_PORT);
    tcpServer.start();
    
    // Create HTTP server for REST API
    HTTPServer httpServer(HTTP_PORT);
    httpServer.start();
    
    fmt::print("\n✓ Gateway started successfully\n");
    fmt::print("  • TCP server: localhost:{}\n", TCP_PORT);
    fmt::print("  • REST API: http://localhost:{}\n", HTTP_PORT);
    fmt::print("\nREST API Endpoints:\n");
    fmt::print("  GET /api/ecus          - List all ECU IDs\n");
    fmt::print("  GET /api/ecus/{{ecuId}}  - Get data for specific ECU\n");
    fmt::print("  GET /api/data           - Get all ECU data\n");
    fmt::print("  GET /health            - Health check\n");
    fmt::print("\nPress Ctrl+C to stop...\n\n");
    
    // Keep running
    std::atomic<bool> running{true};
    
    // Handle shutdown gracefully
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    fmt::print("\nShutting down gateway...\n");
    httpServer.stop();
    tcpServer.stop();
    messageHandler.stop();
    
    fmt::print("✓ Gateway stopped\n");
    
    return 0;
}
