cmake_minimum_required(VERSION 3.15)
project(cpp-messgage-queue)

# Use C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package(fmt REQUIRED)

# Add Conan dependencies
enable_testing()
add_subdirectory(src)
