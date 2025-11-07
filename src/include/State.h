/**
 * @file State.h
 * @brief Base state interface for state machine
 * @author rsikder
 * @date 2024
 */

#pragma once

#include <string>
#include <memory>
#include <any>

/**
 * @brief Forward declaration
 */
class StateMachine;

/**
 * @brief Base interface for all states in the state machine
 * 
 * This is an abstract base class that defines the interface for all state types
 * that can be used in a StateMachine. All custom states must inherit from this
 * class and implement the pure virtual methods.
 * 
 * @note States are typically managed via shared pointers
 * 
 * @see StateMachine
 */
class State {
public:
    virtual ~State() = default;
    
    /**
     * @brief Get the state name/identifier
     * @return String identifier for the state
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Called when entering this state
     * @param context Optional context data passed during transition
     * @param stateMachine Reference to the state machine
     */
    virtual void onEnter(const std::any& context, StateMachine& stateMachine) {}
    
    /**
     * @brief Called when exiting this state
     * @param stateMachine Reference to the state machine
     */
    virtual void onExit(StateMachine& stateMachine) {}
    
    /**
     * @brief Called periodically while in this state (if enabled)
     * @param stateMachine Reference to the state machine
     */
    virtual void onUpdate(StateMachine& stateMachine) {}
    
    /**
     * @brief Handle an event while in this state
     * @param eventName Name of the event
     * @param eventData Optional event data
     * @param stateMachine Reference to the state machine
     * @return True if the event was handled, false otherwise
     */
    virtual bool onEvent(const std::string& eventName, const std::any& eventData, StateMachine& stateMachine) {
        return false;
    }
};

using StatePtr = std::shared_ptr<State>;

