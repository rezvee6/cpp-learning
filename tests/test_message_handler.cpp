#include <gtest/gtest.h>
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/Message.h"
#include "include/MessageTypes.h"
#include <thread>
#include <atomic>
#include <chrono>

class MessageHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<MessageQueue>();
    }

    void TearDown() override {
        if (handler_) {
            handler_->stop();
        }
        queue_->stop();
    }

    std::unique_ptr<MessageQueue> queue_;
    std::unique_ptr<MessageHandler> handler_;
};

// Test basic message processing
TEST_F(MessageHandlerTest, BasicProcessing) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue messages
    for (int i = 0; i < 10; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    handler_->stop();
    
    EXPECT_EQ(processedCount.load(), 10);
}

// Test multiple worker threads
TEST_F(MessageHandlerTest, MultipleWorkers) {
    const int numWorkers = 4;
    handler_ = std::make_unique<MessageHandler>(*queue_, numWorkers);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
            // Simulate processing time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    handler_->start();
    
    // Enqueue many messages
    const int numMessages = 100;
    for (int i = 0; i < numMessages; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait for all messages to be processed
    while (processedCount.load() < numMessages) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    handler_->stop();
    
    EXPECT_EQ(processedCount.load(), numMessages);
}

// Test start/stop
TEST_F(MessageHandlerTest, StartStop) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    EXPECT_FALSE(handler_->isRunning());
    
    handler_->start();
    EXPECT_TRUE(handler_->isRunning());
    
    handler_->stop();
    EXPECT_FALSE(handler_->isRunning());
}

// Test double start
TEST_F(MessageHandlerTest, DoubleStart) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    handler_->start();
    EXPECT_TRUE(handler_->isRunning());
    
    // Starting again should be safe (no-op)
    handler_->start();
    EXPECT_TRUE(handler_->isRunning());
    
    handler_->stop();
}

// Test double stop
TEST_F(MessageHandlerTest, DoubleStop) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    handler_->start();
    handler_->stop();
    EXPECT_FALSE(handler_->isRunning());
    
    // Stopping again should be safe (no-op)
    handler_->stop();
    EXPECT_FALSE(handler_->isRunning());
}

// Test custom message processor
TEST_F(MessageHandlerTest, CustomProcessor) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::vector<std::string> processedIds;
    
    handler_->setMessageProcessor([&processedIds](MessagePtr msg) {
        if (msg) {
            processedIds.push_back(msg->getId());
        }
    });
    
    handler_->start();
    
    // Enqueue different message types
    auto dataMsg = std::make_shared<DataMessage>("data-1", "data");
    auto eventMsg = std::make_shared<EventMessage>(
        "event-1",
        EventMessage::EventType::INFO,
        "test event"
    );
    
    queue_->enqueue(dataMsg);
    queue_->enqueue(eventMsg);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    handler_->stop();
    
    EXPECT_EQ(processedIds.size(), 2);
    EXPECT_EQ(processedIds[0], "data-1");
    EXPECT_EQ(processedIds[1], "event-1");
}

// Test processing after stop
TEST_F(MessageHandlerTest, ProcessingAfterStop) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue some messages
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Stop handler
    handler_->stop();
    
    // Enqueue more messages after stop
    for (int i = 5; i < 10; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Should only process messages enqueued before stop
    EXPECT_LE(processedCount.load(), 5);
}

// Test concurrent access
TEST_F(MessageHandlerTest, ConcurrentAccess) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 2);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Multiple threads enqueueing
    const int numThreads = 4;
    const int messagesPerThread = 50;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([this, t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                auto msg = std::make_shared<DataMessage>(
                    "thread-" + std::to_string(t) + "-msg-" + std::to_string(i),
                    "data"
                );
                queue_->enqueue(msg);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Wait for all messages to be processed
    const int totalMessages = numThreads * messagesPerThread;
    int waitCount = 0;
    while (processedCount.load() < totalMessages && waitCount < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        waitCount++;
    }
    
    handler_->stop();
    
    EXPECT_EQ(processedCount.load(), totalMessages);
}

// Test default message processor (calls msg->process())
TEST_F(MessageHandlerTest, DefaultProcessor) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processCallCount{0};
    
    // Create a custom message that tracks process() calls
    class TestMessage : public Message {
    public:
        TestMessage(std::atomic<int>& count) 
            : count_(count), id_("test"), timestamp_(std::chrono::system_clock::now()) {}
        
        std::string getType() const override { return "TestMessage"; }
        std::string getId() const override { return id_; }
        std::chrono::system_clock::time_point getTimestamp() const override { return timestamp_; }
        
        void process() override {
            count_++;
        }
        
        std::string toString() const override { return "TestMessage"; }
    private:
        std::atomic<int>& count_;
        std::string id_;
        std::chrono::system_clock::time_point timestamp_;
    };
    
    handler_->start();
    
    // Enqueue messages using default processor
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<TestMessage>(processCallCount);
        queue_->enqueue(msg);
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    handler_->stop();
    
    EXPECT_EQ(processCallCount.load(), 5);
}

// Test worker thread behavior when queue is stopped but not empty
TEST_F(MessageHandlerTest, WorkerThreadStoppedWithMessages) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue messages
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Stop handler (which stops queue)
    handler_->stop();
    
    // All messages should still be processed
    EXPECT_EQ(processedCount.load(), 5);
}

// Test MessageHandler destructor
TEST_F(MessageHandlerTest, Destructor) {
    {
        MessageHandler handler(*queue_, 1);
        handler.start();
        
        // Enqueue some messages
        for (int i = 0; i < 3; ++i) {
            auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
            queue_->enqueue(msg);
        }
        
        // Destructor should stop handler
    }
    
    // Handler should be stopped after destruction
    EXPECT_TRUE(queue_->isStopped());
}

// Test MessageHandler with zero workers
TEST_F(MessageHandlerTest, ZeroWorkers) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 0);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue messages
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // With zero workers, nothing should be processed
    EXPECT_EQ(processedCount.load(), 0);
    
    handler_->stop();
}

// Test workerThread when messageProcessor is null
TEST_F(MessageHandlerTest, WorkerThreadNullProcessor) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    // Don't set processor (use default)
    handler_->start();
    
    // Enqueue messages
    for (int i = 0; i < 3; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    
    handler_->stop();
    // Should not crash
}

// Test workerThread when running becomes false and queue is empty
TEST_F(MessageHandlerTest, WorkerThreadStopWhenEmpty) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue one message
    auto msg = std::make_shared<DataMessage>("msg-1", "data");
    queue_->enqueue(msg);
    
    // Wait for it to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Stop handler - worker should exit when queue is empty
    handler_->stop();
    
    EXPECT_EQ(processedCount.load(), 1);
}

// Test start when already running (edge case)
TEST_F(MessageHandlerTest, StartWhenAlreadyRunning) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    handler_->start();
    EXPECT_TRUE(handler_->isRunning());
    
    // Starting again should be safe (no-op)
    handler_->start();
    EXPECT_TRUE(handler_->isRunning());
    
    handler_->stop();
}

// Test stop when not running (edge case)
TEST_F(MessageHandlerTest, StopWhenNotRunning) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    // Stop when not started should be safe
    handler_->stop();
    EXPECT_FALSE(handler_->isRunning());
    
    // Stop again should be safe
    handler_->stop();
    EXPECT_FALSE(handler_->isRunning());
}

// Test workerThread with null message (should not crash)
TEST_F(MessageHandlerTest, WorkerThreadNullMessage) {
    handler_ = std::make_unique<MessageHandler>(*queue_, 1);
    
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
        }
    });
    
    handler_->start();
    
    // Enqueue null message (should be ignored by queue)
    MessagePtr nullMsg = nullptr;
    queue_->enqueue(nullMsg);
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Null message should not be processed
    EXPECT_EQ(processedCount.load(), 0);
    
    handler_->stop();
}

