# Integration Guide & Examples

This document provides comprehensive examples and integration patterns for using the C++ Message Queue & State Machine System in your applications.

## Table of Contents

- [Quick Start Examples](#quick-start-examples)
- [Integration Patterns](#integration-patterns)
- [Real-World Example: ECU Gateway](#real-world-example-ecu-gateway)
- [Advanced Usage](#advanced-usage)

## Quick Start Examples

### Basic Message Queue Usage

```cpp
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/MessageTypes.h"

// Create queue and handler
MessageQueue queue;
MessageHandler handler(queue, 2); // 2 worker threads

// Set up message processor
handler.setMessageProcessor([](MessagePtr msg) {
    if (auto dataMsg = std::dynamic_pointer_cast<DataMessage>(msg)) {
        std::cout << "Processing: " << dataMsg->getData() << std::endl;
    }
    msg->process();
});

// Start processing
handler.start();

// Enqueue messages
auto msg1 = std::make_shared<DataMessage>("id-1", "Hello, World!");
auto msg2 = std::make_shared<DataMessage>("id-2", "Test message");
queue.enqueue(msg1);
queue.enqueue(msg2);

// Wait for processing
std::this_thread::sleep_for(std::chrono::milliseconds(100));

// Stop processing
handler.stop();
```

### Basic State Machine Usage

```cpp
#include "include/StateMachine.h"
#include "include/ExampleStates.h"

// Create state machine
StateMachine sm;

// Add states
sm.addState("init", std::make_shared<InitState>());
sm.addState("active", std::make_shared<ActiveState>());
sm.addState("error", std::make_shared<ErrorState>());

// Define transitions
sm.addTransition("init", "init_complete", "active");
sm.addTransition("active", "error_occurred", "error");
sm.addTransition("error", "recover", "active");

// Set transition callback
sm.setTransitionCallback([](const std::string& from, const std::string& to, const std::any& data) {
    std::cout << "State transition: " << from << " → " << to << std::endl;
});

// Start state machine
sm.setInitialState("init");
sm.start();

// Trigger events to change states
sm.triggerEvent("init_complete");  // init → active
sm.triggerEvent("error_occurred", std::string("Error message"));  // active → error
sm.triggerEvent("recover");  // error → active

// Stop state machine
sm.stop();
```

### Using ECUDataMessage

```cpp
#include "include/MessageTypes.h"

// Create ECU data message
std::map<std::string, std::string> engineData = {
    {"EngineSpeed_RPM.value", "2500"},
    {"EngineSpeed_RPM.unit", "RPM"},
    {"EngineSpeed_RPM.status", "OK"},
    {"CoolantTemperature_C.value", "85.5"},
    {"CoolantTemperature_C.unit", "C"},
    {"CoolantTemperature_C.status", "OK"}
};

auto ecuMsg = std::make_shared<ECUDataMessage>(
    "PCM-ENG-000001",
    "PCM-PowertrainControlModule",
    engineData
);

// Access data
std::cout << "ECU: " << ecuMsg->getECUId() << std::endl;
auto rpm = ecuMsg->getValue("EngineSpeed_RPM.value");
if (rpm.has_value()) {
    std::cout << "RPM: " << rpm.value() << std::endl;
}
```

## Integration Patterns

### Pattern 1: Message Processing Triggers State Transitions

This pattern allows message processing to influence the system's state:

```cpp
MessageQueue queue;
MessageHandler handler(queue, 3);
StateMachine sm;

// Setup state machine
sm.addState("init", std::make_shared<InitState>());
sm.addState("active", std::make_shared<ActiveState>());
sm.addState("error", std::make_shared<ErrorState>());
sm.addTransition("init", "init_complete", "active");
sm.addTransition("active", "error_occurred", "error");
sm.addTransition("error", "recover", "active");

// Integrate: message processor can trigger state transitions
handler.setMessageProcessor([&sm](MessagePtr msg) {
    if (auto eventMsg = std::dynamic_pointer_cast<EventMessage>(msg)) {
        if (eventMsg->getEventType() == EventMessage::EventType::ERROR) {
            // Trigger error state if we're in active state
            if (sm.getCurrentState() == "active") {
                sm.triggerEvent("error_occurred", eventMsg->getDescription());
            }
        }
    }
    msg->process();
});

// Start both systems
sm.setInitialState("init");
sm.start();
handler.start();

// Use both systems...
```

### Pattern 2: State-Based Message Processing

Different states process messages differently:

```cpp
handler.setMessageProcessor([&sm](MessagePtr msg) {
    std::string currentState = sm.getCurrentState();

    if (currentState == "init") {
        // During initialization, only process critical messages
        if (auto eventMsg = std::dynamic_pointer_cast<EventMessage>(msg)) {
            if (eventMsg->getEventType() == EventMessage::EventType::ERROR) {
                msg->process();
            }
        }
    } else if (currentState == "active") {
        // In active state, process all messages
        msg->process();
    } else if (currentState == "error") {
        // In error state, log but don't process
        std::cout << "Error state: Message queued but not processed" << std::endl;
    }
});
```

### Pattern 3: Conditional Transitions with Guards

Use guard functions to make transitions conditional:

```cpp
// Add transition with guard function
sm.addTransition("active", "standby", "standby",
    [](const std::any& data) -> bool {
        // Only transition if data indicates low activity
        if (data.type() == typeid(int)) {
            int messageCount = std::any_cast<int>(data);
            return messageCount < 10; // Transition only if < 10 messages
        }
        return false;
    });

// Trigger with data
int currentMessageCount = 5;
sm.triggerEvent("standby", currentMessageCount); // Will transition if count < 10
```

### Pattern 4: Multi-Threaded Message Processing with State Awareness

```cpp
std::atomic<int> processedCount{0};
std::atomic<bool> systemReady{false};

// State machine callback
sm.setTransitionCallback([&systemReady](const std::string& from,
                                         const std::string& to,
                                         const std::any&) {
    if (to == "active") {
        systemReady = true;
    } else {
        systemReady = false;
    }
});

// Message handler
handler.setMessageProcessor([&processedCount, &systemReady](MessagePtr msg) {
    // Only process if system is ready
    if (systemReady.load()) {
        msg->process();
        processedCount++;
    } else {
        // Queue message for later processing
        std::cout << "System not ready, message queued" << std::endl;
    }
});
```

## Real-World Example: ECU Gateway

The `main.cpp` file contains a complete **ECU Data Gateway** application that demonstrates:

- Receiving vehicle ECU data via TCP socket
- Processing messages with worker threads
- Storing data in memory
- Exposing REST API for data consumption

### Architecture

```
ECU Simulators (TCP Clients)
    ↓
Gateway TCP Server (Port 8080)
    ↓
Message Queue
    ↓
Message Handler (Worker Threads)
    ↓
ECU Data Store (In-Memory)
    ↓
REST API Server (Port 8081)
    ↓
HTTP Clients
```

### Key Components

1. **TCPServer**: Listens for ECU data connections
2. **HTTPServer**: Serves REST API endpoints
3. **ECUDataStore**: Thread-safe storage for latest ECU data
4. **MessageQueue/MessageHandler**: Processes incoming ECU messages

### Running the Example

```bash
# Terminal 1: Start gateway
cd build
./src/cpp-messgage-queue

# Terminal 2: Start simulator
cd build
./tools/ecu_simulator

# Terminal 3: Query API
curl http://localhost:8081/api/data
```

See [TESTING.md](TESTING.md) for detailed testing instructions.

## Advanced Usage

### Custom Message Types

Create custom message types for your domain:

```cpp
class SensorMessage : public Message {
public:
    SensorMessage(const std::string& id, const std::string& sensorType, double value)
        : id_(id), sensorType_(sensorType), value_(value),
          timestamp_(std::chrono::system_clock::now()) {}

    std::string getType() const override { return "SensorMessage"; }
    std::string getId() const override { return id_; }
    std::chrono::system_clock::time_point getTimestamp() const override { return timestamp_; }

    void process() override {
        // Custom processing logic
        std::cout << "Processing sensor: " << sensorType_
                  << " = " << value_ << std::endl;
    }

    std::string toString() const override {
        return fmt::format("[SensorMessage] {}: {} = {}",
                          id_, sensorType_, value_);
    }

    const std::string& getSensorType() const { return sensorType_; }
    double getValue() const { return value_; }

private:
    std::string id_;
    std::string sensorType_;
    double value_;
    std::chrono::system_clock::time_point timestamp_;
};

// Usage
auto sensorMsg = std::make_shared<SensorMessage>("sensor-1", "temperature", 23.5);
queue.enqueue(sensorMsg);
```

### Custom States

Create domain-specific states:

```cpp
class ProcessingState : public State {
public:
    std::string getName() const override { return "processing"; }

    void onEnter(const std::any& context, StateMachine& sm) override {
        std::cout << "Entering processing state" << std::endl;
        // Initialize processing resources
    }

    void onExit(StateMachine& sm) override {
        std::cout << "Exiting processing state" << std::endl;
        // Cleanup resources
    }

    void onUpdate(StateMachine& sm) override {
        // Periodic processing tasks
    }

    bool onEvent(const std::string& event, const std::any& data,
                 StateMachine& sm) override {
        if (event == "pause") {
            sm.triggerEvent("transition_to_paused", data);
            return true;
        }
        return false;
    }
};

// Usage
sm.addState("processing", std::make_shared<ProcessingState>());
```

### Error Handling and Recovery

```cpp
handler.setMessageProcessor([&sm](MessagePtr msg) {
    try {
        msg->process();
    } catch (const std::exception& e) {
        std::cerr << "Error processing message: " << e.what() << std::endl;

        // Trigger error state
        if (sm.getCurrentState() == "active") {
            sm.triggerEvent("error_occurred", std::string(e.what()));
        }
    }
});

// Recovery mechanism
sm.addTransition("error", "recover", "active",
    [](const std::any& data) -> bool {
        // Only recover if error is recoverable
        if (data.type() == typeid(std::string)) {
            std::string error = std::any_cast<std::string>(data);
            return error.find("recoverable") != std::string::npos;
        }
        return false;
    });
```

### Performance Optimization

```cpp
// Use appropriate thread count based on workload
int numThreads = std::thread::hardware_concurrency();
MessageHandler handler(queue, numThreads);

// Batch processing
std::vector<MessagePtr> batch;
while (batch.size() < 100 && !queue.empty()) {
    auto msg = queue.tryDequeue();
    if (msg.has_value()) {
        batch.push_back(msg.value());
    }
}

// Process batch
for (auto& msg : batch) {
    msg->process();
}
```

### Monitoring and Metrics

```cpp
std::atomic<size_t> messagesProcessed{0};
std::atomic<size_t> messagesFailed{0};

handler.setMessageProcessor([&messagesProcessed, &messagesFailed](MessagePtr msg) {
    try {
        msg->process();
        messagesProcessed++;
    } catch (...) {
        messagesFailed++;
    }
});

// Periodically report metrics
std::thread metricsThread([&messagesProcessed, &messagesFailed]() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        std::cout << "Processed: " << messagesProcessed.load()
                  << ", Failed: " << messagesFailed.load() << std::endl;
    }
});
```

## Best Practices

1. **Always use smart pointers** for messages (`std::shared_ptr<Message>`)
2. **Handle exceptions** in message processors
3. **Use appropriate thread counts** (typically CPU cores)
4. **Implement graceful shutdown** (call `stop()` before destruction)
5. **Monitor queue size** to prevent memory issues
6. **Use state machine callbacks** for logging and monitoring
7. **Validate message data** before processing
8. **Use guard functions** for conditional transitions

## See Also

- [README.md](README.md) - API reference and architecture
- [TESTING.md](TESTING.md) - Testing guide
- [PLANNING.md](PLANNING.md) - Future plans and roadmap
