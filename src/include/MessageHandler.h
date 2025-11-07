/**
 * @file MessageHandler.h
 * @brief Multi-threaded message processor
 * @author rsikder
 * @date 2024
 */

#pragma once

#include "MessageQueue.h"
#include <thread>
#include <atomic>
#include <functional>
#include <vector>

/**
 * @brief Handler that processes messages from a queue
 * 
 * This class manages one or more worker threads that continuously process
 * messages from a MessageQueue. It provides a clean interface for starting
 * and stopping message processing, and allows customization of the processing
 * logic via callbacks.
 * 
 * Features:
 * - Configurable number of worker threads
 * - Custom message processing callbacks
 * - Graceful start/stop operations
 * - Automatic thread management
 * 
 * @note The handler must be stopped before destruction to ensure all
 *       worker threads complete properly
 * 
 * @example
 * @code
 * MessageQueue queue;
 * MessageHandler handler(queue, 4); // 4 worker threads
 * 
 * handler.setMessageProcessor([](MessagePtr msg) {
 *     std::cout << "Processing: " << msg->toString() << std::endl;
 *     msg->process();
 * });
 * 
 * handler.start();
 * // ... enqueue messages ...
 * handler.stop();
 * @endcode
 * 
 * @see MessageQueue
 * @see Message
 */
class MessageHandler {
public:
    /**
     * @brief Constructor
     * 
     * Creates a new message handler that will process messages from the
     * given queue using the specified number of worker threads.
     * 
     * @param queue Reference to the MessageQueue to process
     * @param numWorkers Number of worker threads to create (default: 1)
     * 
     * @note The handler is not started automatically. Call start() to begin processing.
     */
    explicit MessageHandler(MessageQueue& queue, size_t numWorkers = 1);
    
    ~MessageHandler();
    
    // Non-copyable
    MessageHandler(const MessageHandler&) = delete;
    MessageHandler& operator=(const MessageHandler&) = delete;
    
    /**
     * @brief Start processing messages
     * 
     * Starts all worker threads and begins processing messages from the queue.
     * If already running, this method does nothing.
     * 
     * @note This method is thread-safe
     */
    void start();
    
    /**
     * @brief Stop processing messages
     * 
     * Stops all worker threads and waits for them to complete. The queue
     * is also stopped to wake up any blocking dequeue operations.
     * 
     * @note This method is thread-safe
     * @note The destructor automatically calls this method
     */
    void stop();
    
    /**
     * @brief Check if handler is running
     * @return True if running
     */
    bool isRunning() const;
    
    /**
     * @brief Set a custom message processor callback
     * 
     * Sets a custom function that will be called for each message processed.
     * If not set, the default behavior is to call msg->process().
     * 
     * @param processor Function that takes a MessagePtr and processes it
     * 
     * @example
     * @code
     * handler.setMessageProcessor([](MessagePtr msg) {
     *     // Custom processing logic
     *     if (auto dataMsg = std::dynamic_pointer_cast<DataMessage>(msg)) {
     *         // Handle DataMessage specifically
     *     }
     *     msg->process();
     * });
     * @endcode
     */
    void setMessageProcessor(std::function<void(MessagePtr)> processor);

private:
    void workerThread();
    
    MessageQueue& queue_;
    std::atomic<bool> running_{false};
    std::vector<std::thread> workers_;
    size_t numWorkers_;
    std::function<void(MessagePtr)> messageProcessor_;
};

