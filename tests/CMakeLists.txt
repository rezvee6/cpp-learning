# Test configuration
enable_testing()

# Find GTest
find_package(GTest REQUIRED)

# Source files for linking (needed for coverage)
set(TEST_SOURCES
    ${CMAKE_SOURCE_DIR}/src/MessageQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/MessageHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/StateMachine.cpp
)

# Test executables
add_executable(test_message_queue
    test_message_queue.cpp
    ${TEST_SOURCES}
)

add_executable(test_message_handler
    test_message_handler.cpp
    ${TEST_SOURCES}
)

add_executable(test_state_machine
    test_state_machine.cpp
    ${TEST_SOURCES}
)

add_executable(test_integration
    test_integration.cpp
    ${TEST_SOURCES}
)

add_executable(test_message_types
    test_message_types.cpp
    ${TEST_SOURCES}
)

# Link all test executables
foreach(test_target test_message_queue test_message_handler test_state_machine test_integration test_message_types)
    target_include_directories(${test_target} PRIVATE 
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/src/include
    )
    
    target_link_libraries(${test_target} PRIVATE
        GTest::gtest
        GTest::gtest_main
        GTest::gmock
        fmt::fmt
    )
    
    # Enable coverage for test targets
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${test_target} PRIVATE 
            -g -O0 --coverage -fprofile-arcs -ftest-coverage
        )
        target_link_libraries(${test_target} PRIVATE --coverage)
    endif()
    
    # Add test to CTest
    add_test(NAME ${test_target} COMMAND ${test_target})
endforeach()

# Set test properties
set_tests_properties(
    test_message_queue
    test_message_handler
    test_state_machine
    test_integration
    test_message_types
    PROPERTIES
    TIMEOUT 60
)

