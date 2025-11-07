#include "include/MessageQueue.h"
#include <chrono>

void MessageQueue::enqueue(MessagePtr message) {
    if (!message) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    if (!stopped_.load()) {
        queue_.push(std::move(message));
        condition_.notify_one();
    }
}

std::optional<MessagePtr> MessageQueue::dequeue() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    // Wait until queue has messages or is stopped
    condition_.wait(lock, [this] {
        return !queue_.empty() || stopped_.load();
    });
    
    if (stopped_.load() && queue_.empty()) {
        return std::nullopt;
    }
    
    if (!queue_.empty()) {
        MessagePtr message = queue_.front();
        queue_.pop();
        return message;
    }
    
    return std::nullopt;
}

std::optional<MessagePtr> MessageQueue::tryDequeue() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return std::nullopt;
    }
    
    MessagePtr message = queue_.front();
    queue_.pop();
    return message;
}

size_t MessageQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool MessageQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

void MessageQueue::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    stopped_.store(true);
    condition_.notify_all();
}

bool MessageQueue::isStopped() const {
    return stopped_.load();
}

void MessageQueue::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    while (!queue_.empty()) {
        queue_.pop();
    }
}

