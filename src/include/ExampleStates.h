/**
 * @file ExampleStates.h
 * @brief Example state implementations for demonstration
 * @author rsikder
 * @date 2024
 */

#pragma once

#include "StateMachine.h"
#include <iostream>
#include <fmt/core.h>

/**
 * @brief Initialization state
 * 
 * This is the starting state where the system performs initialization tasks.
 * Transitions to Active state when initialization is complete.
 */
class InitState : public State {
public:
    std::string getName() const override {
        return "init";
    }
    
    void onEnter(const std::any& context, StateMachine& stateMachine) override {
        fmt::print("[InitState] Entered initialization state\n");
        fmt::print("[InitState] Performing system initialization...\n");
        
        // Example: Extract initialization parameters from context
        try {
            if (context.has_value()) {
                if (auto* config = std::any_cast<std::string>(&context)) {
                    fmt::print("[InitState] Initialization config: {}\n", *config);
                }
            }
        } catch (...) {
            // Context type mismatch, use defaults
        }
        
        // Simulate initialization work
        fmt::print("[InitState] Initialization complete\n");
    }
    
    void onExit(StateMachine& stateMachine) override {
        fmt::print("[InitState] Exiting initialization state\n");
    }
    
    void onUpdate(StateMachine& stateMachine) override {
        // Periodic updates during initialization
        // This could be used to check initialization progress
    }
    
    bool onEvent(const std::string& eventName, const std::any& eventData,
                 StateMachine& stateMachine) override {
        if (eventName == "init_complete") {
            fmt::print("[InitState] Initialization completed, ready to transition\n");
            // Return false to allow the transition to proceed
            return false;
        }
        return false;
    }
};

/**
 * @brief Active state
 * 
 * This is the main operational state where the system is actively processing.
 * Can transition to Error state if an error occurs.
 */
class ActiveState : public State {
public:
    std::string getName() const override {
        return "active";
    }
    
    void onEnter(const std::any& context, StateMachine& stateMachine) override {
        fmt::print("[ActiveState] Entered active state\n");
        fmt::print("[ActiveState] System is now operational and processing\n");
        
        // Example: Extract activation parameters from context
        try {
            if (context.has_value()) {
                if (auto* data = std::any_cast<std::string>(&context)) {
                    fmt::print("[ActiveState] Activation data: {}\n", *data);
                }
            }
        } catch (...) {
            // Context type mismatch, ignore
        }
    }
    
    void onExit(StateMachine& stateMachine) override {
        fmt::print("[ActiveState] Exiting active state\n");
    }
    
    void onUpdate(StateMachine& stateMachine) override {
        // Periodic updates while active
        // This could be used for health checks, monitoring, etc.
    }
    
    bool onEvent(const std::string& eventName, const std::any& eventData,
                 StateMachine& stateMachine) override {
        if (eventName == "heartbeat") {
            fmt::print("[ActiveState] Heartbeat received - system is healthy\n");
            return true;
        } else if (eventName == "pause") {
            fmt::print("[ActiveState] Pause requested\n");
            return true;
        }
        return false;
    }
};

/**
 * @brief Error state
 * 
 * This state is entered when an error occurs. The system can recover
 * from this state and transition back to Init or Active state.
 */
class ErrorState : public State {
public:
    std::string getName() const override {
        return "error";
    }
    
    void onEnter(const std::any& context, StateMachine& stateMachine) override {
        fmt::print("[ErrorState] Entered error state\n");
        
        // Extract error information from context
        std::string errorMessage = "Unknown error";
        try {
            if (context.has_value()) {
                if (auto* msg = std::any_cast<std::string>(&context)) {
                    errorMessage = *msg;
                }
            }
        } catch (...) {
            // Context type mismatch, use default
        }
        
        fmt::print("[ErrorState] Error occurred: {}\n", errorMessage);
        fmt::print("[ErrorState] System is in error recovery mode\n");
    }
    
    void onExit(StateMachine& stateMachine) override {
        fmt::print("[ErrorState] Exiting error state\n");
    }
    
    void onUpdate(StateMachine& stateMachine) override {
        // Periodic updates while in error state
        // This could be used for error recovery attempts, logging, etc.
    }
    
    bool onEvent(const std::string& eventName, const std::any& eventData,
                 StateMachine& stateMachine) override {
        if (eventName == "recover") {
            fmt::print("[ErrorState] Recovery initiated\n");
            // Return false to allow the transition to proceed
            return false;
        } else if (eventName == "retry") {
            fmt::print("[ErrorState] Retry attempt requested\n");
            // Return false to allow retry transitions if configured
            return false;
        }
        return false;
    }
};


