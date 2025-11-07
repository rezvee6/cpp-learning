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
- Generate data from 4 ECUs: engine, transmission, brake, battery
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
{ "ecus": ["engine", "transmission", "brake", "battery"] }
```

#### Get Specific ECU Data

```bash
curl http://localhost:8081/api/ecus/engine
```

Expected response:

```json
{
  "ecuId": "engine",
  "data": {
    "rpm": "2500",
    "temperature": "85.5",
    "pressure": "1.2",
    "throttle_position": "45.0"
  }
}
```

Try other ECUs:

```bash
curl http://localhost:8081/api/ecus/transmission
curl http://localhost:8081/api/ecus/brake
curl http://localhost:8081/api/ecus/battery
```

#### Get All ECU Data

```bash
curl http://localhost:8081/api/data
```

Expected response:

```json
{
  "engine": {
    "rpm": "2500",
    "temperature": "85.5",
    "pressure": "1.2",
    "throttle_position": "45.0"
  },
  "transmission": { "gear": "4", "speed": "60.5", "temperature": "75.0" },
  "brake": { "brake_pressure": "50.0", "abs_active": "true" },
  "battery": {
    "voltage": "12.5",
    "current": "2.3",
    "temperature": "25.0",
    "state_of_charge": "85.0"
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
- Specific ECU: http://localhost:8081/api/ecus/engine

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
