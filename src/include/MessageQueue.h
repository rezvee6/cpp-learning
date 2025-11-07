/**
 * @file MessageQueue.h
 * @brief Thread-safe message queue implementation
 * @author rsikder
 * @date 2024
 */

#pragma once

#include "Message.h"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <optional>

/**
 * @brief Thread-safe message queue for processing messages
 * 
 * This class provides a thread-safe FIFO queue for storing and retrieving
 * messages. It uses mutexes and condition variables to ensure safe concurrent
 * access from multiple threads.
 * 
 * Features:
 * - Thread-safe enqueue/dequeue operations
 * - Blocking and non-blocking dequeue options
 * - Graceful shutdown support
 * - Queue size monitoring
 * 
 * @note This class is non-copyable (deleted copy constructor and assignment)
 * 
 * @example
 * @code
 * MessageQueue queue;
 * auto msg = std::make_shared<DataMessage>("id-1", "data");
 * queue.enqueue(msg);
 * 
 * auto received = queue.dequeue(); // blocking
 * if (received.has_value()) {
 *     received.value()->process();
 * }
 * @endcode
 * 
 * @see Message
 * @see MessageHandler
 */
class MessageQueue {
public:
    MessageQueue() = default;
    ~MessageQueue() = default;
    
    // Non-copyable
    MessageQueue(const MessageQueue&) = delete;
    MessageQueue& operator=(const MessageQueue&) = delete;
    
    /**
     * @brief Push a message to the queue
     * 
     * Adds a message to the end of the queue. This operation is thread-safe
     * and will wake up any threads waiting on dequeue().
     * 
     * @param message Message to enqueue (shared pointer)
     * @note If message is null, the operation is silently ignored
     * @note If the queue is stopped, messages will not be enqueued
     * 
     * @threadsafe Yes
     */
    void enqueue(MessagePtr message);
    
    /**
     * @brief Pop a message from the queue (blocking)
     * 
     * Removes and returns the message at the front of the queue. This method
     * will block until a message is available or the queue is stopped.
     * 
     * @return Message pointer if available, std::nullopt if queue is stopped and empty
     * @note This method will block indefinitely if no messages are available
     *       and the queue is not stopped
     * 
     * @threadsafe Yes
     */
    std::optional<MessagePtr> dequeue();
    
    /**
     * @brief Try to pop a message without blocking
     * 
     * Attempts to remove and return the message at the front of the queue
     * without blocking. Returns immediately if the queue is empty.
     * 
     * @return Message pointer if available, std::nullopt if queue is empty
     * 
     * @threadsafe Yes
     */
    std::optional<MessagePtr> tryDequeue();
    
    /**
     * @brief Get the current size of the queue
     * @return Number of messages in queue
     */
    size_t size() const;
    
    /**
     * @brief Check if the queue is empty
     * @return True if empty
     */
    bool empty() const;
    
    /**
     * @brief Stop the queue (wakes up all waiting threads)
     */
    void stop();
    
    /**
     * @brief Check if the queue is stopped
     * @return True if stopped
     */
    bool isStopped() const;
    
    /**
     * @brief Clear all messages from the queue
     */
    void clear();

private:
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    std::queue<MessagePtr> queue_;
    std::atomic<bool> stopped_{false};
};

