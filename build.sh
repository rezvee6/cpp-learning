
#!/bin/bash

# Ensure the script is executed with a valid argument
if [ -z "$1" ]; then
    echo "Usage: $0 run|test|coverage"
    echo "  run       - Build and run the main application"
    echo "  test      - Build and run tests"
    echo "  coverage  - Build tests with coverage and generate report"
    exit 1
fi

# Define the build directory
BUILD_DIR="build"
COVERAGE_DIR="coverage"

# Create the build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Install Conan dependencies
echo "Installing Conan dependencies..."
conan install . --output-folder=$BUILD_DIR --build=missing

# Configure and build based on command
if [ "$1" = "coverage" ]; then
    # Build with coverage
    echo "Configuring CMake with coverage..."
    cd $BUILD_DIR
    cmake .. -DCMAKE_BUILD_TYPE=Debug \
             -DBUILD_TESTS=ON \
             -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
             -DCMAKE_CXX_FLAGS="--coverage -g -O0" \
             -DCMAKE_EXE_LINKER_FLAGS="--coverage"
    
    echo "Building tests with coverage..."
    cmake --build . --config Debug
    
    echo "Running tests..."
    ctest --output-on-failure --verbose
    
    # Generate coverage report
    echo "Generating coverage report..."
    if command -v lcov &> /dev/null; then
        mkdir -p ../$COVERAGE_DIR/html
        
        lcov --capture --directory . --output-file ../$COVERAGE_DIR/coverage.info \
             --ignore-errors inconsistent,unsupported
        
          lcov --remove ../$COVERAGE_DIR/coverage.info \
             '/usr/*' \
             '*/tests/*' \
             '*/build/*' \
             '*/CMakeFiles/*' \
             '*/.conan/*' \
             '*/conan/*' \
             '*/fmt/*' \
             '*/gtest/*' \
             '*/googletest/*' \
             --output-file ../$COVERAGE_DIR/coverage.info \
             --ignore-errors inconsistent,unsupported
        
        genhtml ../$COVERAGE_DIR/coverage.info \
                --output-directory ../$COVERAGE_DIR/html \
                --ignore-errors inconsistent,unsupported,missing,category ... 
        
        echo ""
        echo "✓ Coverage report generated in $COVERAGE_DIR/html/index.html"
        echo "  Open it in your browser to view the coverage report."
    else
        echo ""
        echo "⚠ lcov not found. Install it to generate HTML coverage reports:"
        echo "  macOS:    brew install lcov"
        echo "  Ubuntu:   sudo apt-get install lcov"
    fi
    cd ..
elif [ "$1" = "test" ]; then
    # Build and run tests
    echo "Configuring and building tests..."
    conan build . --output-folder=$BUILD_DIR
    
    cd $BUILD_DIR
    cmake .. -DBUILD_TESTS=ON -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . --config Release
    
    echo "Running tests..."
    ctest --output-on-failure --verbose
    cd ..
elif [ "$1" = "run" ]; then
    # Build and run main application
    echo "Configuring and building the project..."
    conan build . --output-folder=$BUILD_DIR
    
    cd $BUILD_DIR
    cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake
    cmake --build . --config Release
    
    echo ""
    echo "✓ Build complete!"
    echo ""
    echo "To run the gateway:"
    echo "  ./src/cpp-messgage-queue"
    echo ""
    echo "To run the ECU simulator (in another terminal):"
    echo "  ./tools/ecu_simulator"
    echo ""
    echo "See TESTING.md for detailed testing instructions."
    cd ..
else
    echo "Invalid argument: $1. Use 'run', 'test', or 'coverage'."
    exit 1
fi
