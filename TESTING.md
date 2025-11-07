# Testing the ECU Gateway

This guide explains how to test the ECU Data Gateway application.

## Prerequisites

1. Build the project:

```bash
./build.sh run
```

Or manually:

```bash
cd build
conan install .. --output-folder=. --build=missing
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
cmake --build .
```

## Testing Steps

### Step 1: Start the Gateway

In your first terminal, run the gateway:

```bash
cd build
./src/cpp-messgage-queue
```

You should see:

```
╔══════════════════════════════════════════════════════════╗
║              ECU Data Gateway                            ║
║     Receives ECU data and exposes REST API               ║
╚══════════════════════════════════════════════════════════╝

✓ TCP server listening on port 8080 for ECU data
✓ HTTP server listening on port 8081 for REST API

✓ Gateway started successfully
  • TCP server: localhost:8080
  • REST API: http://localhost:8081

REST API Endpoints:
  GET /api/ecus          - List all ECU IDs
  GET /api/ecus/{ecuId}  - Get data for specific ECU
  GET /api/data           - Get all ECU data
  GET /health            - Health check

Press Ctrl+C to stop...
```

### Step 2: Start the ECU Simulator

In a **second terminal**, run the ECU simulator:

```bash
cd build
./tools/ecu_simulator
```

Or with custom parameters:

```bash
./tools/ecu_simulator [host] [port] [duration_seconds] [interval_ms]
# Example:
./tools/ecu_simulator 127.0.0.1 8080 30 500
```

The simulator will:

- Connect to the gateway on port 8080
- Generate data from 4 ECUs: PCM-PowertrainControlModule, TCM-TransmissionControlModule, BCM-BrakeControlModule, BMS-BatteryManagementSystem
- Send data every second (or custom interval)
- Run for 60 seconds (or custom duration)

### Step 3: Test the REST API

In a **third terminal** (or use the same terminal as the simulator), test the REST API endpoints:

#### Health Check

```bash
curl http://localhost:8081/health
```

Expected response:

```json
{ "status": "ok", "service": "ECU Gateway" }
```

#### List All ECUs

```bash
curl http://localhost:8081/api/ecus
```

Expected response (after simulator sends data):

```json
{
  "ecus": [
    "PCM-PowertrainControlModule",
    "TCM-TransmissionControlModule",
    "BCM-BrakeControlModule",
    "BMS-BatteryManagementSystem"
  ]
}
```

#### Get Specific ECU Data

```bash
curl http://localhost:8081/api/ecus/PCM-PowertrainControlModule
```

Expected response:

```json
{
  "ecuId": "PCM-PowertrainControlModule",
  "data": {
    "EngineSpeed_RPM": {
      "value": 2500,
      "unit": "RPM",
      "status": "OK",
      "timestamp": "2024-01-15T10:30:45.123Z"
    },
    "CoolantTemperature_C": {
      "value": 85.5,
      "unit": "C",
      "status": "OK",
      "timestamp": "2024-01-15T10:30:45.123Z"
    },
    ...
  }
}
```

Try other ECUs:

```bash
curl http://localhost:8081/api/ecus/TCM-TransmissionControlModule
curl http://localhost:8081/api/ecus/BCM-BrakeControlModule
curl http://localhost:8081/api/ecus/BMS-BatteryManagementSystem
```

#### Get All ECU Data

```bash
curl http://localhost:8081/api/data
```

Expected response:

```json
{
  "PCM-PowertrainControlModule": {
    "EngineSpeed_RPM": {
      "value": 2500,
      "unit": "RPM",
      "status": "OK",
      "timestamp": "2024-01-15T10:30:45.123Z"
    },
    "CoolantTemperature_C": {
      "value": 85.5,
      "unit": "C",
      "status": "OK",
      "timestamp": "2024-01-15T10:30:45.123Z"
    },
    ...
  },
  "TCM-TransmissionControlModule": {
    "CurrentGear": { "value": 4, "unit": "-", "status": "OK", "timestamp": "..." },
    "VehicleSpeed_kmh": { "value": 60.5, "unit": "km/h", "status": "OK", "timestamp": "..." },
    ...
  },
  "BCM-BrakeControlModule": {
    "FrontBrakePressure_kPa": { "value": 5000.0, "unit": "kPa", "status": "OK", "timestamp": "..." },
    "ABSStatus": { "value": "INACTIVE", "unit": "-", "status": "INACTIVE", "timestamp": "..." },
    ...
  },
  "BMS-BatteryManagementSystem": {
    "BatteryVoltage_V": { "value": 12.5, "unit": "V", "status": "OK", "timestamp": "..." },
    "BatteryCurrent_A": { "value": 2.3, "unit": "A", "status": "OK", "timestamp": "..." },
    ...
  }
}
```

#### Test Non-existent ECU

```bash
curl http://localhost:8081/api/ecus/nonexistent
```

Expected response:

```json
{ "error": 404, "message": "ECU not found" }
```

## Testing with Browser

You can also test the REST API in your browser:

- Health: http://localhost:8081/health
- List ECUs: http://localhost:8081/api/ecus
- All Data: http://localhost:8081/api/data
- Specific ECU: http://localhost:8081/api/ecus/PCM-PowertrainControlModule

## Continuous Testing

To continuously monitor the data, you can use `watch`:

```bash
watch -n 1 'curl -s http://localhost:8081/api/data | python3 -m json.tool'
```

Or use a simple loop:

```bash
while true; do
  echo "=== $(date) ==="
  curl -s http://localhost:8081/api/data | python3 -m json.tool
  echo ""
  sleep 2
done
```

## Stopping the Services

1. Stop the simulator: Press `Ctrl+C` in the simulator terminal
2. Stop the gateway: Press `Ctrl+C` in the gateway terminal

## Troubleshooting

### Port Already in Use

If you get "Address already in use" errors:

```bash
# Find process using port 8080
lsof -i :8080
# Find process using port 8081
lsof -i :8081

# Kill the process (replace PID with actual process ID)
kill -9 <PID>
```

### Connection Refused

- Make sure the gateway is running before starting the simulator
- Check that ports 8080 and 8081 are not blocked by firewall

### No Data in API

- Make sure the simulator is running and connected
- Wait a few seconds for data to arrive
- Check the gateway terminal for error messages

## Stress Testing

The project includes a stress test tool to test the gateway under high load.

### Running Stress Tests

1. **Start the gateway** (in Terminal 1):

   ```bash
   cd build
   ./src/cpp-messgage-queue
   ```

2. **Run the stress test** (in Terminal 2):

   ```bash
   cd build
   ./tools/stress_test [connections] [msgs_per_conn] [interval_ms] [http_duration] [http_rps]
   ```

   Default parameters:

   - `connections`: 10 concurrent TCP connections
   - `msgs_per_conn`: 100 messages per connection
   - `interval_ms`: 10ms between messages (100 msg/s per connection)
   - `http_duration`: 30 seconds of HTTP load testing
   - `http_rps`: 50 requests per second

### Example Stress Test Scenarios

**Light Load:**

```bash
./tools/stress_test 5 50 50 10 10
```

**Medium Load:**

```bash
./tools/stress_test 10 100 10 30 50
```

**Heavy Load:**

```bash
./tools/stress_test 50 200 5 60 100
```

**Extreme Load:**

```bash
./tools/stress_test 100 500 1 120 200
```

### What the Stress Test Does

1. **TCP Load Testing:**

   - Creates multiple concurrent connections to the gateway
   - Sends ECU data messages at high frequency
   - Monitors success/failure rates

2. **HTTP Load Testing:**

   - Concurrently tests all REST API endpoints
   - Tests: `/health`, `/api/ecus`, `/api/data`, and individual ECU endpoints
   - Measures response times and success rates

3. **Metrics Reported:**
   - Total messages sent
   - Failed messages
   - Success rate percentage
   - HTTP requests made
   - HTTP failures
   - HTTP success rate

### Quick Stress Test Runner

Use the provided script for interactive stress testing:

```bash
./run_stress_test.sh
```

This will prompt you to select a pre-configured scenario or create a custom test.
