#include <gtest/gtest.h>
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/MessageTypes.h"
#include "include/StateMachine.h"
#include "include/ExampleStates.h"
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        queue_ = std::make_unique<MessageQueue>();
        handler_ = std::make_unique<MessageHandler>(*queue_, 2);
        sm_ = std::make_unique<StateMachine>();
        
        // Setup state machine
        sm_->addState("init", std::make_shared<InitState>());
        sm_->addState("active", std::make_shared<ActiveState>());
        sm_->addState("error", std::make_shared<ErrorState>());
        
        sm_->addTransition("init", "init_complete", "active");
        sm_->addTransition("active", "error_occurred", "error");
        sm_->addTransition("error", "recover", "active");
        
        sm_->setInitialState("init");
    }

    void TearDown() override {
        if (handler_ && handler_->isRunning()) {
            handler_->stop();
        }
        if (sm_ && sm_->isRunning()) {
            sm_->stop();
        }
        queue_->stop();
    }

    std::unique_ptr<MessageQueue> queue_;
    std::unique_ptr<MessageHandler> handler_;
    std::unique_ptr<StateMachine> sm_;
};

// Test integration: message processing triggers state transition
TEST_F(IntegrationTest, MessageTriggersStateTransition) {
    std::atomic<int> processedCount{0};
    std::atomic<bool> errorStateReached{false};
    
    // Setup message processor that triggers state transitions
    handler_->setMessageProcessor([this, &processedCount, &errorStateReached](MessagePtr msg) {
        if (!msg) return;
        
        processedCount++;
        
        if (auto eventMsg = std::dynamic_pointer_cast<EventMessage>(msg)) {
            if (eventMsg->getEventType() == EventMessage::EventType::ERROR) {
                if (sm_->getCurrentState() == "active") {
                    sm_->triggerEvent("error_occurred", eventMsg->getDescription());
                    errorStateReached = true;
                }
            }
        }
    });
    
    sm_->start();
    handler_->start();
    
    // Start in init, transition to active
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Enqueue normal messages
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("data-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Enqueue error message that should trigger state transition
    auto errorMsg = std::make_shared<EventMessage>(
        "error-1",
        EventMessage::EventType::ERROR,
        "Test error"
    );
    queue_->enqueue(errorMsg);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    EXPECT_GE(processedCount.load(), 6);
    EXPECT_TRUE(errorStateReached.load());
    EXPECT_EQ(sm_->getCurrentState(), "error");
}

// Test full lifecycle: init -> active -> error -> recover
TEST_F(IntegrationTest, FullLifecycle) {
    sm_->start();
    
    // Init -> Active
    EXPECT_EQ(sm_->getCurrentState(), "init");
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Active -> Error
    sm_->triggerEvent("error_occurred", std::string("Database error"));
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    // Error -> Active (recover)
    sm_->triggerEvent("recover");
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

// Test concurrent message processing with state machine
TEST_F(IntegrationTest, ConcurrentProcessing) {
    std::atomic<int> processedCount{0};
    
    handler_->setMessageProcessor([&processedCount](MessagePtr msg) {
        if (msg) {
            processedCount++;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    sm_->start();
    sm_->triggerEvent("init_complete");
    handler_->start();
    
    // Multiple threads enqueueing
    const int numThreads = 4;
    const int messagesPerThread = 25;
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
    
    // Wait for processing
    const int totalMessages = numThreads * messagesPerThread;
    int waitCount = 0;
    while (processedCount.load() < totalMessages && waitCount < 100) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        waitCount++;
    }
    
    EXPECT_EQ(processedCount.load(), totalMessages);
}

// Test state machine state during message processing
TEST_F(IntegrationTest, StateDuringProcessing) {
    std::mutex statesMutex;
    std::vector<std::string> statesDuringProcessing;
    
    handler_->setMessageProcessor([this, &statesDuringProcessing, &statesMutex](MessagePtr msg) {
        if (msg) {
            std::lock_guard<std::mutex> lock(statesMutex);
            statesDuringProcessing.push_back(sm_->getCurrentState());
        }
    });
    
    sm_->start();
    
    // Start in init
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    // Transition to active
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Start handler after state transition
    handler_->start();
    
    // Enqueue messages
    for (int i = 0; i < 5; ++i) {
        auto msg = std::make_shared<DataMessage>("msg-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Stop handler before checking results
    handler_->stop();
    
    // All messages should be processed while in "active" state
    {
        std::lock_guard<std::mutex> lock(statesMutex);
        EXPECT_GE(statesDuringProcessing.size(), 5);
        for (const auto& state : statesDuringProcessing) {
            EXPECT_EQ(state, "active");
        }
    }
}

// Test error recovery scenario
TEST_F(IntegrationTest, ErrorRecovery) {
    std::atomic<int> errorCount{0};
    std::atomic<int> recoveryCount{0};
    
    handler_->setMessageProcessor([this, &errorCount, &recoveryCount](MessagePtr msg) {
        if (auto eventMsg = std::dynamic_pointer_cast<EventMessage>(msg)) {
            if (eventMsg->getEventType() == EventMessage::EventType::ERROR) {
                errorCount++;
                if (sm_->getCurrentState() == "active") {
                    sm_->triggerEvent("error_occurred", eventMsg->getDescription());
                }
            }
        }
    });
    
    sm_->start();
    sm_->triggerEvent("init_complete");
    handler_->start();
    
    // Enqueue error
    auto errorMsg = std::make_shared<EventMessage>(
        "error-1",
        EventMessage::EventType::ERROR,
        "Recoverable error"
    );
    queue_->enqueue(errorMsg);
    
    // Wait for error to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    // Recover
    sm_->triggerEvent("recover");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    recoveryCount++;
    
    // Process more messages after recovery
    for (int i = 0; i < 3; ++i) {
        auto msg = std::make_shared<DataMessage>("recovered-" + std::to_string(i), "data");
        queue_->enqueue(msg);
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_GE(errorCount.load(), 1);
    EXPECT_EQ(recoveryCount.load(), 1);
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

