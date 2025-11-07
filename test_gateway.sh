#!/bin/bash

# Quick test script for the ECU Gateway

echo "Building project..."
./build.sh run

echo ""
echo "=========================================="
echo "Gateway Testing Instructions:"
echo "=========================================="
echo ""
echo "1. In this terminal, start the gateway:"
echo "   cd build && ./src/cpp-messgage-queue"
echo ""
echo "2. In a NEW terminal, start the simulator:"
echo "   cd build && ./tools/ecu_simulator"
echo ""
echo "3. In a THIRD terminal (or use the simulator terminal), test the API:"
echo "   curl http://localhost:8081/health"
echo "   curl http://localhost:8081/api/ecus"
echo "   curl http://localhost:8081/api/data"
echo "   curl http://localhost:8081/api/ecus/engine"
echo ""
echo "See TESTING.md for more details!"
echo ""
