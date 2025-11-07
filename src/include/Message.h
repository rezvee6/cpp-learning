/**
 * @file Message.h
 * @brief Base message interface for the message queue system
 * @author rsikder
 * @date 2024
 */

#pragma once

#include <memory>
#include <string>
#include <chrono>

/**
 * @brief Base interface for all messages in the queue
 * 
 * This is an abstract base class that defines the interface for all message types
 * that can be processed by the MessageQueue and MessageHandler. All custom message
 * types must inherit from this class and implement the pure virtual methods.
 * 
 * @note Messages are typically managed via shared pointers (MessagePtr)
 * 
 * @see MessageQueue
 * @see MessageHandler
 * @see DataMessage
 * @see EventMessage
 */
class Message {
public:
    virtual ~Message() = default;
    
    /**
     * @brief Get the message type identifier
     * @return String identifier for the message type
     */
    virtual std::string getType() const = 0;
    
    /**
     * @brief Get the message ID (unique identifier)
     * @return Message ID
     */
    virtual std::string getId() const = 0;
    
    /**
     * @brief Get the timestamp when the message was created
     * @return Timestamp
     */
    virtual std::chrono::system_clock::time_point getTimestamp() const = 0;
    
    /**
     * @brief Process the message (called by handler)
     */
    virtual void process() = 0;
    
    /**
     * @brief Get a string representation of the message
     * @return String representation
     */
    virtual std::string toString() const = 0;
};

/**
 * @brief Type alias for shared pointer to Message
 * 
 * This is the recommended way to pass messages around in the system,
 * as it provides automatic memory management and allows multiple
 * references to the same message.
 */
using MessagePtr = std::shared_ptr<Message>;

