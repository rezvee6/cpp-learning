

# Ensure the script is executed with a valid argument
if [ -z "$1" ]; then
    echo "Usage: $0 srun|test"
    exit 1
fi

# Define the build directory
BUILD_DIR="build"

# Create the build directory if it doesn't exist
mkdir -p $BUILD_DIR

# Install Conan dependencies
echo "Installing Conan dependencies..."
conan install . --output-folder=$BUILD_DIR --build=missing

# Configure and build the project
echo "Configuring and building the project..."
conan build . --output-folder=$BUILD_DIR

# Move into the build directory
cd $BUILD_DIR

# Execute based on the first argument
if [ "$1" = "test" ]; then
    # Run tests
    echo "Running tests..."
    ctest --verbose
elif [ "$1" = "run" ]; then
    # Run the main application
    echo "Running the main application..."
    ./build/src/cpp-messgage-queue
else
    echo "Invalid argument: $1. Use 'run' or 'test'."
    exit 1
fi
