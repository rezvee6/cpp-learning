/**
 * @file MessageTypes.h
 * @brief Example message type implementations
 * @author rsikder
 * @date 2024
 */

#pragma once

#include "Message.h"
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <map>
#include <optional>

/**
 * @brief Example message type for data ingestion
 * 
 * This is a concrete implementation of the Message interface designed
 * for handling data ingestion scenarios. It stores arbitrary string data
 * along with metadata (ID, timestamp).
 * 
 * @example
 * @code
 * auto msg = std::make_shared<DataMessage>("msg-1", "Hello, World!");
 * queue.enqueue(msg);
 * @endcode
 * 
 * @see Message
 * @see EventMessage
 */
class DataMessage : public Message {
public:
    /**
     * @brief Constructor
     * @param id Unique identifier for this message
     * @param data The data payload to store
     */
    DataMessage(const std::string& id, const std::string& data)
        : id_(id), data_(data), timestamp_(std::chrono::system_clock::now()) {}
    
    std::string getType() const override {
        return "DataMessage";
    }
    
    std::string getId() const override {
        return id_;
    }
    
    std::chrono::system_clock::time_point getTimestamp() const override {
        return timestamp_;
    }
    
    void process() override {
        // Process the data message
        // This is where you would implement your data processing logic
    }
    
    std::string toString() const override {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
        std::stringstream ss;
        ss << "[DataMessage] ID: " << id_ 
           << ", Data: " << data_
           << ", Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    /**
     * @brief Get the data payload
     * @return Reference to the stored data string
     */
    const std::string& getData() const {
        return data_;
    }

private:
    std::string id_;
    std::string data_;
    std::chrono::system_clock::time_point timestamp_;
};

/**
 * @brief Example message type for system events
 * 
 * This is a concrete implementation of the Message interface designed
 * for handling system events with different severity levels (INFO, WARNING, ERROR).
 * 
 * @example
 * @code
 * auto event = std::make_shared<EventMessage>(
 *     "event-1",
 *     EventMessage::EventType::ERROR,
 *     "Database connection failed"
 * );
 * queue.enqueue(event);
 * @endcode
 * 
 * @see Message
 * @see DataMessage
 */
class EventMessage : public Message {
public:
    /**
     * @brief Event severity levels
     */
    enum class EventType {
        INFO,      ///< Informational event
        WARNING,   ///< Warning event
        ERROR      ///< Error event
    };
    
    /**
     * @brief Constructor
     * @param id Unique identifier for this event
     * @param type Event severity level
     * @param description Human-readable event description
     */
    EventMessage(const std::string& id, EventType type, const std::string& description)
        : id_(id), type_(type), description_(description), 
          timestamp_(std::chrono::system_clock::now()) {}
    
    std::string getType() const override {
        return "EventMessage";
    }
    
    std::string getId() const override {
        return id_;
    }
    
    std::chrono::system_clock::time_point getTimestamp() const override {
        return timestamp_;
    }
    
    void process() override {
        // Handle event based on type
        // This is where you would implement your event handling logic
    }
    
    std::string toString() const override {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
        std::string typeStr;
        switch (type_) {
            case EventType::INFO: typeStr = "INFO"; break;
            case EventType::WARNING: typeStr = "WARNING"; break;
            case EventType::ERROR: typeStr = "ERROR"; break;
        }
        
        std::stringstream ss;
        ss << "[EventMessage] ID: " << id_
           << ", Type: " << typeStr
           << ", Description: " << description_
           << ", Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    /**
     * @brief Get the event type/severity
     * @return The EventType enum value
     */
    EventType getEventType() const {
        return type_;
    }
    
    /**
     * @brief Get the event description
     * @return Reference to the description string
     */
    const std::string& getDescription() const {
        return description_;
    }

private:
    std::string id_;
    EventType type_;
    std::string description_;
    std::chrono::system_clock::time_point timestamp_;
};

/**
 * @brief Message type for vehicle ECU data
 * 
 * This message type is designed for vehicle Electronic Control Unit (ECU) data,
 * containing structured information from various ECUs in a vehicle.
 * 
 * @example
 * @code
 * auto ecuMsg = std::make_shared<ECUDataMessage>(
 *     "ecu-1",
 *     "engine",
 *     {{"rpm", "2500"}, {"temperature", "85"}, {"pressure", "1.2"}}
 * );
 * queue.enqueue(ecuMsg);
 * @endcode
 * 
 * @see Message
 * @see DataMessage
 */
class ECUDataMessage : public Message {
public:
    /**
     * @brief Constructor
     * @param id Unique identifier for this message
     * @param ecuId Identifier of the ECU (e.g., "engine", "transmission", "brakes")
     * @param data Key-value pairs of sensor/parameter data
     */
    ECUDataMessage(const std::string& id, const std::string& ecuId, 
                   const std::map<std::string, std::string>& data)
        : id_(id), ecuId_(ecuId), data_(data), 
          timestamp_(std::chrono::system_clock::now()) {}
    
    std::string getType() const override {
        return "ECUDataMessage";
    }
    
    std::string getId() const override {
        return id_;
    }
    
    std::chrono::system_clock::time_point getTimestamp() const override {
        return timestamp_;
    }
    
    void process() override {
        // Process ECU data message
        // This is where you would implement ECU-specific processing logic
    }
    
    std::string toString() const override {
        auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
        std::stringstream ss;
        ss << "[ECUDataMessage] ID: " << id_ 
           << ", ECU: " << ecuId_
           << ", Data: {";
        
        bool first = true;
        for (const auto& [key, value] : data_) {
            if (!first) ss << ", ";
            ss << key << "=" << value;
            first = false;
        }
        ss << "}"
           << ", Timestamp: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }
    
    /**
     * @brief Get the ECU identifier
     * @return ECU ID string
     */
    const std::string& getECUId() const {
        return ecuId_;
    }
    
    /**
     * @brief Get the data map
     * @return Reference to the data map
     */
    const std::map<std::string, std::string>& getData() const {
        return data_;
    }
    
    /**
     * @brief Get a specific data value by key
     * @param key The parameter key
     * @return Optional value if key exists
     */
    std::optional<std::string> getValue(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    std::string id_;
    std::string ecuId_;
    std::map<std::string, std::string> data_;
    std::chrono::system_clock::time_point timestamp_;
};

