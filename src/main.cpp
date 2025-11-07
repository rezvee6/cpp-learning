#include <iostream>
#include <fmt/core.h>
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/MessageTypes.h"
#include "include/StateMachine.h"
#include "include/ExampleStates.h"
#include <thread>
#include <chrono>
#include <random>
#include <atomic>

/**
 * @brief Real-world example: Data Ingestion Service
 * 
 * This example demonstrates a data ingestion service that:
 * 1. Starts in Init state (initializing connections, loading config)
 * 2. Transitions to Active state (ready to process incoming data)
 * 3. Processes messages from a queue using worker threads
 * 4. Handles errors gracefully (transitions to Error state)
 * 5. Recovers from errors and continues processing
 * 
 * This combines both the message queue system and state machine
 * to create a realistic, production-like scenario.
 */

// Global state for the service
std::atomic<int> processedCount{0};
std::atomic<int> errorCount{0};
std::atomic<bool> serviceRunning{false};

int main() {
    fmt::print("╔══════════════════════════════════════════════════════════╗\n");
    fmt::print("║     Data Ingestion Service - Real-World Example         ║\n");
    fmt::print("║     Combining Message Queue + State Machine             ║\n");
    fmt::print("╚══════════════════════════════════════════════════════════╝\n\n");
    
    // ========================================================================
    // Phase 1: Initialize the System
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 1: System Initialization\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // Create message queue and handler
    MessageQueue messageQueue;
    MessageHandler messageHandler(messageQueue, 3); // 3 worker threads
    
    // Create state machine
    StateMachine stateMachine;
    
    // Add states
    stateMachine.addState("init", std::make_shared<InitState>());
    stateMachine.addState("active", std::make_shared<ActiveState>());
    stateMachine.addState("error", std::make_shared<ErrorState>());
    
    // Define state transitions
    stateMachine.addTransition("init", "init_complete", "active");
    stateMachine.addTransition("active", "error_occurred", "error");
    stateMachine.addTransition("error", "recover", "init");
    stateMachine.addTransition("error", "recover_to_active", "active");
    
    // Set transition callback
    stateMachine.setTransitionCallback([](const std::string& from, const std::string& to, const std::string&) {
        fmt::print("\n[STATE MACHINE] Transition: {} → {}\n", from, to);
    });
    
    // Start in Init state
    stateMachine.setInitialState("init");
    stateMachine.start();
    
    fmt::print("✓ State machine initialized in '{}' state\n", stateMachine.getCurrentState());
    fmt::print("✓ Message queue created\n");
    fmt::print("✓ Message handler created (3 worker threads)\n\n");
    
    // ========================================================================
    // Phase 2: Initialize System Components
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 2: Component Initialization\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    fmt::print("Initializing components...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    fmt::print("  • Database connection pool: OK\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fmt::print("  • Configuration loaded: OK\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fmt::print("  • Message queue ready: OK\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    fmt::print("  • Worker threads ready: OK\n\n");
    
    // Transition to Active state
    fmt::print("Completing initialization...\n");
    stateMachine.triggerEvent("init_complete", std::string("All systems ready"));
    fmt::print("✓ System is now in '{}' state\n\n", stateMachine.getCurrentState());
    
    // ========================================================================
    // Phase 3: Start Processing Messages
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 3: Start Message Processing\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // Set up custom message processor that integrates with state machine
    messageHandler.setMessageProcessor([&stateMachine](MessagePtr msg) {
        if (!msg) return;
        
        // Process the message
        auto msgType = msg->getType();
        auto msgId = msg->getId();
        
        fmt::print("[WORKER] Processing {} (ID: {})\n", msgType, msgId);
        
        // Simulate processing time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Check for errors in certain messages
        if (auto eventMsg = std::dynamic_pointer_cast<EventMessage>(msg)) {
            if (eventMsg->getEventType() == EventMessage::EventType::ERROR) {
                errorCount++;
                fmt::print("[WORKER] ⚠ Error detected in message: {}\n", 
                          eventMsg->getDescription());
                
                // Trigger error state if we're in active state
                if (stateMachine.getCurrentState() == "active") {
                    fmt::print("[WORKER] Triggering error state transition...\n");
                    stateMachine.triggerEvent("error_occurred", 
                                             eventMsg->getDescription());
                }
            }
        } else if (auto dataMsg = std::dynamic_pointer_cast<DataMessage>(msg)) {
            // Simulate data processing
            processedCount++;
            fmt::print("[WORKER] ✓ Processed data: {}\n", dataMsg->getData());
        }
    });
    
    // Start the message handler
    messageHandler.start();
    serviceRunning = true;
    fmt::print("✓ Message handler started\n");
    fmt::print("✓ Service is now processing messages\n\n");
    
    // ========================================================================
    // Phase 4: Simulate Data Ingestion
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 4: Ingesting Data\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    // Simulate incoming data from external sources
    fmt::print("Simulating data ingestion from external sources...\n\n");
    
    // Batch 1: Normal data
    fmt::print("Batch 1: Normal data ingestion\n");
    for (int i = 1; i <= 5; ++i) {
        auto msg = std::make_shared<DataMessage>(
            fmt::format("data-{:03d}", i),
            fmt::format("Sensor reading #{}: temperature=22.5°C, humidity=65%", i)
        );
        messageQueue.enqueue(msg);
        fmt::print("  [INGEST] Enqueued: {}\n", msg->getId());
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Batch 2: Mix of data and events
    fmt::print("\nBatch 2: Mixed data and events\n");
    for (int i = 6; i <= 8; ++i) {
        auto msg = std::make_shared<DataMessage>(
            fmt::format("data-{:03d}", i),
            fmt::format("Sensor reading #{}: pressure=1013.25 hPa", i)
        );
        messageQueue.enqueue(msg);
        fmt::print("  [INGEST] Enqueued: {}\n", msg->getId());
    }
    
    // Add an info event
    auto infoEvent = std::make_shared<EventMessage>(
        "event-info-1",
        EventMessage::EventType::INFO,
        "Batch processing started"
    );
    messageQueue.enqueue(infoEvent);
    fmt::print("  [INGEST] Enqueued: {}\n", infoEvent->getId());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    
    // Batch 3: Error scenario
    fmt::print("\nBatch 3: Error scenario\n");
    for (int i = 9; i <= 10; ++i) {
        auto msg = std::make_shared<DataMessage>(
            fmt::format("data-{:03d}", i),
            fmt::format("Sensor reading #{}: voltage=3.3V", i)
        );
        messageQueue.enqueue(msg);
        fmt::print("  [INGEST] Enqueued: {}\n", msg->getId());
    }
    
    // Add an error event that will trigger state transition
    auto errorEvent = std::make_shared<EventMessage>(
        "event-error-1",
        EventMessage::EventType::ERROR,
        "Database connection lost - retrying..."
    );
    messageQueue.enqueue(errorEvent);
    fmt::print("  [INGEST] Enqueued: {} (ERROR)\n", errorEvent->getId());
    
    // Wait for error to be processed
    std::this_thread::sleep_for(std::chrono::milliseconds(800));
    
    fmt::print("\nCurrent system state: {}\n", stateMachine.getCurrentState());
    fmt::print("Messages processed: {}\n", processedCount.load());
    fmt::print("Errors encountered: {}\n\n", errorCount.load());
    
    // ========================================================================
    // Phase 5: Error Recovery
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 5: Error Recovery\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    if (stateMachine.getCurrentState() == "error") {
        fmt::print("System is in error state. Attempting recovery...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        fmt::print("  • Reconnecting to database...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        fmt::print("  • Verifying connections...\n");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        fmt::print("  • Recovery successful!\n\n");
        
        // Recover directly to active state
        stateMachine.triggerEvent("recover_to_active", 
                                 std::string("Database reconnected"));
        fmt::print("✓ System recovered and returned to '{}' state\n\n", 
                  stateMachine.getCurrentState());
    }
    
    // ========================================================================
    // Phase 6: Continue Processing
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 6: Continue Processing\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    fmt::print("Continuing to process remaining messages...\n\n");
    
    // Add more data after recovery
    for (int i = 11; i <= 15; ++i) {
        auto msg = std::make_shared<DataMessage>(
            fmt::format("data-{:03d}", i),
            fmt::format("Sensor reading #{}: timestamp={}", i, 
                       std::chrono::duration_cast<std::chrono::seconds>(
                           std::chrono::system_clock::now().time_since_epoch()).count())
        );
        messageQueue.enqueue(msg);
    }
    
    // Wait for all messages to be processed
    fmt::print("Waiting for queue to empty...\n");
    int waitCount = 0;
    while (!messageQueue.empty() && waitCount < 50) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waitCount++;
    }
    
    // ========================================================================
    // Phase 7: System Summary
    // ========================================================================
    fmt::print("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 7: System Summary\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    fmt::print("Final Statistics:\n");
    fmt::print("  • Current State: {}\n", stateMachine.getCurrentState());
    fmt::print("  • Messages Processed: {}\n", processedCount.load());
    fmt::print("  • Errors Encountered: {}\n", errorCount.load());
    fmt::print("  • Queue Size: {}\n", messageQueue.size());
    fmt::print("  • Service Running: {}\n\n", serviceRunning.load() ? "Yes" : "No");
    
    // Show state history
    auto history = stateMachine.getStateHistory();
    fmt::print("State History:\n");
    fmt::print("  ");
    for (size_t i = 0; i < history.size(); ++i) {
        fmt::print("{}", history[i]);
        if (i < history.size() - 1) {
            fmt::print(" → ");
        }
    }
    fmt::print("\n\n");
    
    // ========================================================================
    // Phase 8: Shutdown
    // ========================================================================
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
    fmt::print("PHASE 8: Graceful Shutdown\n");
    fmt::print("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n");
    
    fmt::print("Shutting down service...\n");
    serviceRunning = false;
    
    // Stop message handler
    messageHandler.stop();
    fmt::print("✓ Message handler stopped\n");
    
    // Stop state machine
    stateMachine.stop();
    fmt::print("✓ State machine stopped\n");
    
    fmt::print("\n╔══════════════════════════════════════════════════════════╗\n");
    fmt::print("║           Service Shutdown Complete                      ║\n");
    fmt::print("╚══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
