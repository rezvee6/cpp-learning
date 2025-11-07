# ECU Simulator Tool
add_executable(ecu_simulator
    ecu_simulator.cpp
)

# Include directories
target_include_directories(ecu_simulator PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# Link fmt library
target_link_libraries(ecu_simulator fmt::fmt)

# Stress Test Tool
add_executable(stress_test
    stress_test.cpp
)

# Include directories
target_include_directories(stress_test PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
)

# Link fmt library
target_link_libraries(stress_test fmt::fmt)

