#include <gtest/gtest.h>
#include "include/MessageQueue.h"
#include "include/MessageTypes.h"
#include <thread>
#include <vector>
#include <chrono>

class MessageQueueTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<MessageQueue>();
    }

    void TearDown() override {
        queue_->stop();
    }

    std::unique_ptr<MessageQueue> queue_;
};

// Test basic enqueue and dequeue
TEST_F(MessageQueueTest, BasicEnqueueDequeue) {
    auto msg = std::make_shared<DataMessage>("test-1", "test data");
    queue_->enqueue(msg);
    
    EXPECT_FALSE(queue_->empty());
    EXPECT_EQ(queue_->size(), 1);
    
    auto received = queue_->dequeue();
    ASSERT_TRUE(received.has_value());
    EXPECT_EQ(received.value()->getId(), "test-1");
    EXPECT_TRUE(queue_->empty());
}

// Test tryDequeue (non-blocking)
TEST_F(MessageQueueTest, TryDequeue) {
    // Try dequeue when empty
    auto result = queue_->tryDequeue();
    EXPECT_FALSE(result.has_value());
    
    // Enqueue and try dequeue
    auto msg = std::make_shared<DataMessage>("test-2", "data");
    queue_->enqueue(msg);
    
    result = queue_->tryDequeue();
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value()->getId(), "test-2");
}

// Test FIFO ordering
TEST_F(MessageQueueTest, FIFOOrdering) {
    for (int i = 1; i <= 5; ++i) {
        auto msg = std::make_shared<DataMessage>(
            "msg-" + std::to_string(i),
            "data-" + std::to_string(i)
        );
        queue_->enqueue(msg);
    }
    
    for (int i = 1; i <= 5; ++i) {
        auto received = queue_->dequeue();
        ASSERT_TRUE(received.has_value());
        EXPECT_EQ(received.value()->getId(), "msg-" + std::to_string(i));
    }
}

// Test queue size
TEST_F(MessageQueueTest, QueueSize) {
    EXPECT_EQ(queue_->size(), 0);
    EXPECT_TRUE(queue_->empty());
    
    for (int i = 0; i < 10; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    EXPECT_EQ(queue_->size(), 10);
    EXPECT_FALSE(queue_->empty());
}

// Test stop functionality
TEST_F(MessageQueueTest, StopQueue) {
    EXPECT_FALSE(queue_->isStopped());
    
    queue_->stop();
    EXPECT_TRUE(queue_->isStopped());
    
    // Should not be able to enqueue after stop
    auto msg = std::make_shared<DataMessage>("test", "data");
    queue_->enqueue(msg);
    EXPECT_EQ(queue_->size(), 0); // Message should not be enqueued
    
    // Dequeue should return nullopt when stopped and empty
    auto result = queue_->dequeue();
    EXPECT_FALSE(result.has_value());
}

// Test clear functionality
TEST_F(MessageQueueTest, ClearQueue) {
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    EXPECT_EQ(queue_->size(), 5);
    queue_->clear();
    EXPECT_EQ(queue_->size(), 0);
    EXPECT_TRUE(queue_->empty());
}

// Test null message handling
TEST_F(MessageQueueTest, NullMessageHandling) {
    MessagePtr nullMsg = nullptr;
    queue_->enqueue(nullMsg);
    
    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

// Test thread safety - multiple producers
TEST_F(MessageQueueTest, ThreadSafetyMultipleProducers) {
    const int numThreads = 4;
    const int messagesPerThread = 100;
    
    std::vector<std::thread> threads;
    
    // Start producer threads
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
    
    // Wait for all producers
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(queue_->size(), numThreads * messagesPerThread);
}

// Test thread safety - producer and consumer
TEST_F(MessageQueueTest, ThreadSafetyProducerConsumer) {
    const int numMessages = 1000;
    std::atomic<int> receivedCount{0};
    
    // Consumer thread
    std::thread consumer([this, &receivedCount, numMessages]() {
        int count = 0;
        while (count < numMessages) {
            auto msg = queue_->dequeue();
            if (msg.has_value()) {
                receivedCount++;
                count++;
            }
        }
    });
    
    // Producer thread
    std::thread producer([this, numMessages]() {
        for (int i = 0; i < numMessages; ++i) {
            auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
            queue_->enqueue(msg);
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });
    
    producer.join();
    consumer.join();
    
    EXPECT_EQ(receivedCount.load(), numMessages);
}

// Test blocking dequeue
TEST_F(MessageQueueTest, BlockingDequeue) {
    std::atomic<bool> messageReceived{false};
    
    // Consumer thread that blocks waiting for message
    std::thread consumer([this, &messageReceived]() {
        auto msg = queue_->dequeue();
        if (msg.has_value()) {
            messageReceived = true;
        }
    });
    
    // Give consumer time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(messageReceived.load());
    
    // Enqueue message - should wake up consumer
    auto msg = std::make_shared<DataMessage>("wakeup", "data");
    queue_->enqueue(msg);
    
    // Wait for consumer
    consumer.join();
    EXPECT_TRUE(messageReceived.load());
}

// Test dequeue when stopped with messages in queue
TEST_F(MessageQueueTest, DequeueWhenStoppedWithMessages) {
    // Enqueue messages
    for (int i = 0; i < 3; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    EXPECT_EQ(queue_->size(), 3);
    
    // Stop the queue
    queue_->stop();
    
    // Should still be able to dequeue existing messages
    int dequeued = 0;
    while (true) {
        auto msg = queue_->dequeue();
        if (!msg.has_value()) {
            break;
        }
        dequeued++;
    }
    
    EXPECT_EQ(dequeued, 3);
    EXPECT_TRUE(queue_->empty());
}

// Test dequeue when stopped and empty
TEST_F(MessageQueueTest, DequeueWhenStoppedAndEmpty) {
    queue_->stop();
    
    // Should return nullopt immediately
    auto msg = queue_->dequeue();
    EXPECT_FALSE(msg.has_value());
}

// Test enqueue after stop
TEST_F(MessageQueueTest, EnqueueAfterStop) {
    queue_->stop();
    
    // Enqueue should be ignored
    auto msg = std::make_shared<DataMessage>("test", "data");
    queue_->enqueue(msg);
    
    EXPECT_TRUE(queue_->empty());
    EXPECT_EQ(queue_->size(), 0);
}

// Test dequeue when queue becomes empty but not stopped
TEST_F(MessageQueueTest, DequeueEmptyButNotStopped) {
    // Enqueue and dequeue all
    for (int i = 0; i < 3; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    while (!queue_->empty()) {
        queue_->dequeue();
    }
    
    EXPECT_TRUE(queue_->empty());
    
    // Now try dequeue - should block until message arrives or stop
    std::atomic<bool> dequeued{false};
    std::thread consumer([this, &dequeued]() {
        auto msg = queue_->dequeue();
        if (msg.has_value()) {
            dequeued = true;
        }
    });
    
    // Give it time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(dequeued.load());
    
    // Enqueue a message
    auto msg = std::make_shared<DataMessage>("wakeup", "data");
    queue_->enqueue(msg);
    
    consumer.join();
    EXPECT_TRUE(dequeued.load());
}

// Test dequeue when stopped but queue has messages (edge case)
TEST_F(MessageQueueTest, DequeueStoppedWithMessagesEdgeCase) {
    // Enqueue messages
    for (int i = 0; i < 2; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Stop queue
    queue_->stop();
    
    // Dequeue should still work for existing messages
    int count = 0;
    while (true) {
        auto msg = queue_->dequeue();
        if (!msg.has_value()) {
            break;
        }
        count++;
    }
    
    EXPECT_EQ(count, 2);
}

// Test dequeue edge case - queue empty after wait condition
TEST_F(MessageQueueTest, DequeueEmptyAfterWait) {
    // This tests the edge case where queue becomes empty
    // between the wait condition check and the actual dequeue
    std::atomic<bool> gotMessage{false};
    
    std::thread consumer([this, &gotMessage]() {
        auto msg = queue_->dequeue();
        if (msg.has_value()) {
            gotMessage = true;
        }
    });
    
    // Give consumer time to block
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Enqueue and immediately stop
    auto msg = std::make_shared<DataMessage>("test", "data");
    queue_->enqueue(msg);
    queue_->stop();
    
    consumer.join();
    EXPECT_TRUE(gotMessage.load());
}

