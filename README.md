# C++ Message Queue & State Machine System

A comprehensive C++ library combining a thread-safe message queue system with an event-driven state machine for building robust, production-ready applications.

## Documentation

- **[README.md](README.md)** - Overview, architecture, and API reference (this file)
- **[INTEGRATION.md](INTEGRATION.md)** - Integration examples, patterns, and real-world usage
- **[TESTING.md](TESTING.md)** - Testing guide and stress testing instructions
- **[PLANNING.md](PLANNING.md)** - Future goals and roadmap

## Overview

This library provides two integrated systems:

### Message Queue System

- **Thread-safe operations**: All queue operations are protected by mutexes and condition variables
- **Multi-threaded processing**: Configurable worker threads for parallel message processing
- **Extensible design**: Easy to create custom message types by inheriting from the `Message` base class
- **Graceful shutdown**: Clean start/stop mechanisms for production use
- **Built-in message types**: `DataMessage`, `EventMessage`, and `ECUDataMessage` for common use cases

### State Machine System

- **Event-driven transitions**: Trigger state changes based on events
- **State lifecycle callbacks**: `onEnter()`, `onExit()`, `onUpdate()`, and `onEvent()` hooks
- **Transition guards**: Conditional transitions with guard functions
- **State history tracking**: Monitor state transitions over time
- **Thread-safe**: All operations are protected for concurrent access

### Common Features

- **Modern C++**: Uses C++17 features including `std::optional`, smart pointers, and threading primitives
- **Production-ready**: Designed for real-world applications with error handling and recovery
- **Example application**: Complete ECU Data Gateway demonstrating real-world usage

## Architecture

The system consists of two main subsystems that work together:

### Message Queue System

1. **Message**: Abstract base class defining the interface for all message types
2. **MessageQueue**: Thread-safe FIFO queue for storing messages
3. **MessageHandler**: Multi-threaded processor that consumes messages from the queue

### State Machine System

1. **State**: Abstract base class defining the interface for all state types
2. **StateMachine**: Manages states, transitions, and events
3. **Example States**: `InitState`, `ActiveState`, `ErrorState` (provided as examples)

### Class Diagram

```
Message Queue System:
Message (abstract)
    ↑
    ├── DataMessage
    ├── EventMessage
    └── ECUDataMessage

MessageQueue
    ↓ (uses)
MessageHandler

State Machine System:
State (abstract)
    ↑
    ├── InitState
    ├── ActiveState
    └── ErrorState

StateMachine
    ↓ (manages)
State instances
```

### Integration

The systems can work independently or together. The example in `main.cpp` demonstrates an **ECU Data Gateway** that:

- Receives vehicle ECU data via TCP socket
- Uses the message queue to process incoming ECU messages
- Stores and exposes ECU data via REST API
- Demonstrates real-world usage of the message queue system

## Quick Start

For detailed examples and integration patterns, see [INTEGRATION.md](INTEGRATION.md).

### Basic Usage

```cpp
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include "include/MessageTypes.h"

// Create and start message queue system
MessageQueue queue;
MessageHandler handler(queue, 2);
handler.start();

// Enqueue messages
auto msg = std::make_shared<DataMessage>("id-1", "Hello, World!");
queue.enqueue(msg);

// Stop when done
handler.stop();
```

### State Machine

```cpp
#include "include/StateMachine.h"
#include "include/ExampleStates.h"

StateMachine sm;
sm.addState("init", std::make_shared<InitState>());
sm.addState("active", std::make_shared<ActiveState>());
sm.addTransition("init", "init_complete", "active");
sm.setInitialState("init");
sm.start();
sm.triggerEvent("init_complete");  // init → active
```

See [INTEGRATION.md](INTEGRATION.md) for comprehensive examples and patterns.

## Building

### Requirements

- CMake 3.15 or higher
- C++17 compatible compiler
- Conan (for dependency management)

### Build Steps

```bash
# Install dependencies
conan install . --output-folder=build --build=missing

# Build
conan build . --output-folder=build

# Run
./build/src/cpp-messgage-queue
```

Or use the provided build script:

```bash
./build.sh run
```

## Testing

### Running Tests

The project includes comprehensive unit tests using Google Test (GTest).

**Run tests:**

```bash
./build.sh test
```

**Run tests with coverage:**

```bash
./build.sh coverage
```

This will:

1. Build all tests with coverage enabled
2. Run all unit tests
3. Generate a coverage report (if lcov is installed)

### Test Coverage

To view coverage reports:

1. Install lcov (if not already installed):

   ```bash
   # macOS
   brew install lcov

   # Ubuntu/Debian
   sudo apt-get install lcov
   ```

2. Run tests with coverage:

   ```bash
   ./build.sh coverage
   ```

3. Open the HTML report:
   ```bash
   open coverage/html/index.html  # macOS
   xdg-open coverage/html/index.html  # Linux
   ```

### Test Structure

- `tests/test_message_queue.cpp` - MessageQueue unit tests
- `tests/test_message_handler.cpp` - MessageHandler unit tests
- `tests/test_state_machine.cpp` - StateMachine unit tests
- `tests/test_message_types.cpp` - Message type tests (including ECUDataMessage)
- `tests/test_integration.cpp` - Integration tests for both systems
- `tests/test_gateway.cpp` - Gateway functionality tests

## Documentation

### Generating Documentation

This project uses [Doxygen](https://www.doxygen.nl/) for API documentation generation.

#### Install Doxygen

**macOS:**

```bash
brew install doxygen graphviz
```

**Ubuntu/Debian:**

```bash
sudo apt-get install doxygen graphviz
```

**Windows:**
Download from [Doxygen website](https://www.doxygen.nl/download.html)

#### Generate Documentation

```bash
# Generate HTML documentation
doxygen Doxyfile

# Documentation will be in docs/html/index.html
```

Or use the provided script:

```bash
./generate_docs.sh
```

### Viewing Documentation

After generation, open `docs/html/index.html` in your web browser.

## API Reference

### MessageQueue

Thread-safe message queue for storing and retrieving messages.

**Key Methods:**

- `void enqueue(MessagePtr message)` - Add message to queue
- `std::optional<MessagePtr> dequeue()` - Blocking dequeue
- `std::optional<MessagePtr> tryDequeue()` - Non-blocking dequeue
- `size_t size() const` - Get queue size
- `bool empty() const` - Check if empty
- `void stop()` - Stop the queue

### MessageHandler

Multi-threaded message processor.

**Key Methods:**

- `void start()` - Start processing messages
- `void stop()` - Stop processing
- `bool isRunning() const` - Check if running
- `void setMessageProcessor(std::function<void(MessagePtr)> processor)` - Set custom processor

### StateMachine

Event-driven state machine for managing application states.

**Key Methods:**

- `bool addState(const std::string& name, StatePtr state)` - Add a state
- `bool addTransition(const std::string& from, const std::string& event, const std::string& to)` - Add transition
- `bool setInitialState(const std::string& name)` - Set initial state
- `bool start()` - Start the state machine
- `void stop()` - Stop the state machine
- `bool triggerEvent(const std::string& event, const std::any& data)` - Trigger state transition
- `std::string getCurrentState() const` - Get current state name
- `std::vector<std::string> getStateHistory(size_t maxHistory)` - Get state history
- `void setTransitionCallback(callback)` - Set callback for transitions

### Message Types

#### DataMessage

For data ingestion scenarios.

```cpp
auto msg = std::make_shared<DataMessage>("id", "data payload");
```

#### EventMessage

For system events with severity levels.

```cpp
auto event = std::make_shared<EventMessage>(
    "event-id",
    EventMessage::EventType::ERROR,
    "description"
);
```

#### ECUDataMessage

For vehicle Electronic Control Unit (ECU) data with structured key-value pairs.

```cpp
std::map<std::string, std::string> data = {
    {"rpm", "2500"},
    {"temperature", "85.5"},
    {"pressure", "1.2"}
};
auto ecuMsg = std::make_shared<ECUDataMessage>("ecu-1", "engine", data);
```

### State Types

#### InitState

Initialization state - system performs setup tasks.

#### ActiveState

Active operational state - system is processing normally.

#### ErrorState

Error state - system has encountered an error and can recover.

## Creating Custom Types

### Custom Message Types

To create a custom message type, inherit from `Message`:

```cpp
class CustomMessage : public Message {
public:
    CustomMessage(const std::string& id, /* your params */)
        : id_(id), /* initialize */ {}

    std::string getType() const override {
        return "CustomMessage";
    }

    std::string getId() const override {
        return id_;
    }

    std::chrono::system_clock::time_point getTimestamp() const override {
        return timestamp_;
    }

    void process() override {
        // Your processing logic
    }

    std::string toString() const override {
        // String representation
    }

private:
    std::string id_;
    std::chrono::system_clock::time_point timestamp_;
    // Your members
};
```

### Custom State Types

To create a custom state, inherit from `State`:

```cpp
class CustomState : public State {
public:
    std::string getName() const override {
        return "custom";
    }

    void onEnter(const std::any& context, StateMachine& sm) override {
        // Called when entering this state
    }

    void onExit(StateMachine& sm) override {
        // Called when exiting this state
    }

    void onUpdate(StateMachine& sm) override {
        // Called periodically while in this state
    }

    bool onEvent(const std::string& event, const std::any& data,
                 StateMachine& sm) override {
        // Handle events specific to this state
        return false; // Return true if event was handled
    }
};
```

## Integration

The systems can work independently or together. See [INTEGRATION.md](INTEGRATION.md) for:

- Integration patterns and examples
- Real-world usage scenarios
- Advanced patterns and best practices
- Complete ECU Gateway example walkthrough

## ECU Gateway Application

The project includes a complete **ECU Data Gateway** application demonstrating real-world usage.

**Quick Start:**

```bash
# Terminal 1: Start gateway
cd build && ./src/cpp-messgage-queue

# Terminal 2: Start simulator
cd build && ./tools/ecu_simulator

# Terminal 3: Query API
curl http://localhost:8081/api/data
```

**Features:**

- Receives ECU data via TCP (port 8080)
- REST API for data consumption (port 8081)
- Realistic vehicle ECU data with proper units, status, and timestamps

**ECUs:** PCM-PowertrainControlModule, TCM-TransmissionControlModule, BCM-BrakeControlModule, BMS-BatteryManagementSystem

For complete walkthrough, architecture details, and usage examples, see [INTEGRATION.md](INTEGRATION.md).  
For testing instructions, see [TESTING.md](TESTING.md).

## Thread Safety

All public methods of `MessageQueue`, `MessageHandler`, and `StateMachine` are thread-safe and can be called from multiple threads concurrently.

## Future Plans

See [PLANNING.md](PLANNING.md) for future goals and planned features, including:

- Raspberry Pi deployment and performance testing
- Performance optimizations
- Enhanced features and capabilities
- Technical debt items

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Author

rsikder
