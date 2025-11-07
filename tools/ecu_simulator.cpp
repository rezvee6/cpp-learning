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
#include <iomanip>
#include <ctime>

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

    std::string getISOTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z";
        return ss.str();
    }

    std::string getStatus(double value, double min, double max, double warnMin, double warnMax) {
        if (value < min || value > max) return "ERROR";
        if (value < warnMin || value > warnMax) return "WARNING";
        return "OK";
    }

    std::string generateEngineData(int sequence) {
        std::uniform_real_distribution<double> rpmDist(800, 6000);
        std::uniform_real_distribution<double> coolantTempDist(75, 105);
        std::uniform_real_distribution<double> intakePressureDist(30, 150); // kPa
        std::uniform_real_distribution<double> throttleDist(0, 100);
        std::uniform_real_distribution<double> oilTempDist(80, 120);
        std::uniform_real_distribution<double> fuelLevelDist(10, 100);

        int rpm = static_cast<int>(rpmDist(engine_rng_));
        double coolantTemp = coolantTempDist(engine_rng_);
        double intakePressure = intakePressureDist(engine_rng_);
        double throttle = throttleDist(engine_rng_);
        double oilTemp = oilTempDist(engine_rng_);
        double fuelLevel = fuelLevelDist(engine_rng_);

        std::string timestamp = getISOTimestamp();
        std::string rpmStatus = getStatus(rpm, 0, 6500, 100, 6000);
        std::string coolantStatus = getStatus(coolantTemp, 60, 110, 80, 100);
        std::string throttleStatus = "OK";

        return fmt::format(
            R"({{"id":"PCM-ENG-{:06d}","ecuId":"PCM-PowertrainControlModule","timestamp":"{}","data":{{"EngineSpeed_RPM":{{"value":{},"unit":"RPM","status":"{}","timestamp":"{}"}},"CoolantTemperature_C":{{"value":{:.1f},"unit":"C","status":"{}","timestamp":"{}"}},"IntakeManifoldPressure_kPa":{{"value":{:.1f},"unit":"kPa","status":"OK","timestamp":"{}"}},"ThrottlePosition_Percent":{{"value":{:.1f},"unit":"%","status":"OK","timestamp":"{}"}},"EngineOilTemperature_C":{{"value":{:.1f},"unit":"C","status":"OK","timestamp":"{}"}},"FuelLevel_Percent":{{"value":{:.1f},"unit":"%","status":"OK","timestamp":"{}"}}}}}})",
            sequence, timestamp, rpm, rpmStatus, timestamp, 
            coolantTemp, coolantStatus, timestamp,
            intakePressure, timestamp,
            throttle, timestamp,
            oilTemp, timestamp,
            fuelLevel, timestamp
        );
    }

    std::string generateTransmissionData(int sequence) {
        std::uniform_int_distribution<int> gearDist(0, 8);
        std::uniform_real_distribution<double> speedDist(0, 150);
        std::uniform_real_distribution<double> tempDist(60, 95);
        std::uniform_real_distribution<double> torqueDist(50, 400);

        int gear = gearDist(transmission_rng_);
        double speed = speedDist(transmission_rng_);
        double temp = tempDist(transmission_rng_);
        double torque = torqueDist(transmission_rng_);

        std::string timestamp = getISOTimestamp();
        std::string tempStatus = getStatus(temp, 50, 100, 70, 90);
        std::string gearStatus = (gear == 0) ? "NEUTRAL" : "OK";

        return fmt::format(
            R"({{"id":"TCM-TRX-{:06d}","ecuId":"TCM-TransmissionControlModule","timestamp":"{}","data":{{"CurrentGear":{{"value":{},"unit":"-","status":"{}","timestamp":"{}"}},"VehicleSpeed_kmh":{{"value":{:.1f},"unit":"km/h","status":"OK","timestamp":"{}"}},"TransmissionFluidTemp_C":{{"value":{:.1f},"unit":"C","status":"{}","timestamp":"{}"}},"TransmissionTorque_Nm":{{"value":{:.1f},"unit":"Nm","status":"OK","timestamp":"{}"}},"GearPosition":{{"value":"{}","unit":"-","status":"OK","timestamp":"{}"}}}}}})",
            sequence, timestamp, gear, gearStatus, timestamp,
            speed, timestamp,
            temp, tempStatus, timestamp,
            torque, timestamp,
            (gear == 0 ? "NEUTRAL" : (gear == 1 ? "DRIVE" : "GEAR_" + std::to_string(gear))), timestamp
        );
    }

    std::string generateBrakeData(int sequence) {
        std::uniform_real_distribution<double> frontPressureDist(0, 12000); // kPa
        std::uniform_real_distribution<double> rearPressureDist(0, 10000);
        std::uniform_int_distribution<int> absDist(0, 1);
        std::uniform_int_distribution<int> ebdDist(0, 1);
        std::uniform_real_distribution<double> brakeTempDist(20, 150);

        double frontPressure = frontPressureDist(brake_rng_);
        double rearPressure = rearPressureDist(brake_rng_);
        bool absActive = absDist(brake_rng_) == 1;
        bool ebdActive = ebdDist(brake_rng_) == 1;
        double brakeTemp = brakeTempDist(brake_rng_);

        std::string timestamp = getISOTimestamp();
        std::string absStatus = absActive ? "ACTIVE" : "INACTIVE";
        std::string pressureStatus = (frontPressure > 10000 || rearPressure > 8000) ? "WARNING" : "OK";

        return fmt::format(
            R"({{"id":"BCM-BRK-{:06d}","ecuId":"BCM-BrakeControlModule","timestamp":"{}","data":{{"FrontBrakePressure_kPa":{{"value":{:.1f},"unit":"kPa","status":"{}","timestamp":"{}"}},"RearBrakePressure_kPa":{{"value":{:.1f},"unit":"kPa","status":"OK","timestamp":"{}"}},"ABSStatus":{{"value":"{}","unit":"-","status":"{}","timestamp":"{}"}},"EBDActive":{{"value":"{}","unit":"-","status":"OK","timestamp":"{}"}},"BrakeDiscTemperature_C":{{"value":{:.1f},"unit":"C","status":"OK","timestamp":"{}"}}}}}})",
            sequence, timestamp, frontPressure, pressureStatus, timestamp,
            rearPressure, timestamp,
            (absActive ? "ACTIVE" : "INACTIVE"), absStatus, timestamp,
            (ebdActive ? "TRUE" : "FALSE"), timestamp,
            brakeTemp, timestamp
        );
    }

    std::string generateBatteryData(int sequence) {
        std::uniform_real_distribution<double> voltageDist(11.8, 14.2);
        std::uniform_real_distribution<double> currentDist(-60, 80);
        std::uniform_real_distribution<double> tempDist(15, 40);
        std::uniform_real_distribution<double> socDist(25, 100);
        std::uniform_real_distribution<double> healthDist(80, 100);

        double voltage = voltageDist(battery_rng_);
        double current = currentDist(battery_rng_);
        double temp = tempDist(battery_rng_);
        double soc = socDist(battery_rng_);
        double health = healthDist(battery_rng_);

        std::string timestamp = getISOTimestamp();
        std::string voltageStatus = getStatus(voltage, 11.5, 14.5, 12.0, 14.0);
        std::string socStatus = getStatus(soc, 20, 100, 30, 100);
        std::string tempStatus = getStatus(temp, 0, 50, 10, 35);
        std::string healthStatus = getStatus(health, 0, 100, 70, 100);

        return fmt::format(
            R"({{"id":"BMS-BAT-{:06d}","ecuId":"BMS-BatteryManagementSystem","timestamp":"{}","data":{{"BatteryVoltage_V":{{"value":{:.2f},"unit":"V","status":"{}","timestamp":"{}"}},"BatteryCurrent_A":{{"value":{:.2f},"unit":"A","status":"OK","timestamp":"{}"}},"BatteryTemperature_C":{{"value":{:.1f},"unit":"C","status":"{}","timestamp":"{}"}},"StateOfCharge_Percent":{{"value":{:.1f},"unit":"%","status":"{}","timestamp":"{}"}},"BatteryHealth_Percent":{{"value":{:.1f},"unit":"%","status":"{}","timestamp":"{}"}}}}}})",
            sequence, timestamp, voltage, voltageStatus, timestamp,
            current, timestamp,
            temp, tempStatus, timestamp,
            soc, socStatus, timestamp,
            health, healthStatus, timestamp
        );
    }

    void run(int durationSeconds = 60, int intervalMs = 1000) {
        running_ = true;
        int sequence = 0;
        auto startTime = std::chrono::steady_clock::now();

        fmt::print("Starting ECU simulator...\n");
        fmt::print("  Duration: {} seconds\n", durationSeconds);
        fmt::print("  Interval: {} ms\n", intervalMs);
        fmt::print("  ECUs: PCM-PowertrainControlModule, TCM-TransmissionControlModule, BCM-BrakeControlModule, BMS-BatteryManagementSystem\n\n");

        while (running_) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
            
            if (elapsed >= durationSeconds) {
                break;
            }

            // Generate data from all ECUs
            std::vector<std::pair<std::string, std::string>> ecuData = {
                {"PCM-PowertrainControlModule", generateEngineData(sequence)},
                {"TCM-TransmissionControlModule", generateTransmissionData(sequence)},
                {"BCM-BrakeControlModule", generateBrakeData(sequence)},
                {"BMS-BatteryManagementSystem", generateBatteryData(sequence)}
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

