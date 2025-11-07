
# Source files
set(SOURCES
    main.cpp
    MessageQueue.cpp
    MessageHandler.cpp
    StateMachine.cpp
)

# Add executable
add_executable(cpp-messgage-queue ${SOURCES})

# Include directories
target_include_directories(cpp-messgage-queue PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Link fmt library
target_link_libraries(cpp-messgage-queue fmt::fmt)
        
