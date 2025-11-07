#include "include/MessageHandler.h"
#include <iostream>

MessageHandler::MessageHandler(MessageQueue& queue, size_t numWorkers)
    : queue_(queue), numWorkers_(numWorkers) {
    // Default processor just calls the message's process method
    messageProcessor_ = [](MessagePtr msg) {
        if (msg) {
            msg->process();
        }
    };
}

MessageHandler::~MessageHandler() {
    stop();
}

void MessageHandler::start() {
    if (running_.load()) {
        return;
    }
    
    running_.store(true);
    workers_.clear();
    
    for (size_t i = 0; i < numWorkers_; ++i) {
        workers_.emplace_back(&MessageHandler::workerThread, this);
    }
}

void MessageHandler::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    queue_.stop();
    
    // Wait for all workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    workers_.clear();
}

bool MessageHandler::isRunning() const {
    return running_.load();
}

void MessageHandler::setMessageProcessor(std::function<void(MessagePtr)> processor) {
    messageProcessor_ = std::move(processor);
}

void MessageHandler::workerThread() {
    while (running_.load() || !queue_.empty()) {
        auto message = queue_.dequeue();
        
        if (!message.has_value()) {
            // Queue is stopped and empty
            break;
        }
        
        if (messageProcessor_) {
            messageProcessor_(message.value());
        }
    }
}

