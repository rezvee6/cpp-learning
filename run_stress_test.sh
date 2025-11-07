#!/bin/bash

echo "╔══════════════════════════════════════════════════════════╗"
echo "║        ECU Gateway Stress Test Runner                    ║"
echo "╚══════════════════════════════════════════════════════════╝"
echo ""
echo "Make sure the gateway is running in another terminal!"
echo "Press Enter to continue or Ctrl+C to cancel..."
read

cd build

if [ ! -f "./tools/stress_test" ]; then
    echo "Error: stress_test not found. Building..."
    cmake --build . --target stress_test
fi

echo ""
echo "Select stress test scenario:"
echo "1. Light Load (5 connections, 50 msgs, 50ms interval)"
echo "2. Medium Load (10 connections, 100 msgs, 10ms interval)"
echo "3. Heavy Load (50 connections, 200 msgs, 5ms interval)"
echo "4. Extreme Load (100 connections, 500 msgs, 1ms interval)"
echo "5. Custom"
echo ""
read -p "Choice [1-5]: " choice

case $choice in
    1)
        ./tools/stress_test 5 50 50 10 10
        ;;
    2)
        ./tools/stress_test 10 100 10 30 50
        ;;
    3)
        ./tools/stress_test 50 200 5 60 100
        ;;
    4)
        ./tools/stress_test 100 500 1 120 200
        ;;
    5)
        read -p "Connections: " conn
        read -p "Messages per connection: " msgs
        read -p "Interval (ms): " interval
        read -p "HTTP duration (s): " duration
        read -p "HTTP requests/s: " rps
        ./tools/stress_test $conn $msgs $interval $duration $rps
        ;;
    *)
        echo "Invalid choice"
        exit 1
        ;;
esac
