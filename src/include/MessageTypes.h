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

