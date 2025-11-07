/**
 * @file StateMachine.h
 * @brief State machine implementation with event-driven transitions
 * @author rsikder
 * @date 2024
 */

#pragma once

#include "State.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <functional>
#include <any>
#include <optional>

/**
 * @brief State machine for managing state transitions
 * 
 * This class provides a thread-safe state machine implementation that supports:
 * - Event-driven state transitions
 * - State entry/exit callbacks
 * - State update callbacks
 * - Transition validation
 * - State history tracking
 * 
 * @note This class is thread-safe for concurrent access
 * 
 * @example
 * @code
 * StateMachine sm;
 * sm.addState("idle", std::make_shared<IdleState>());
 * sm.addState("running", std::make_shared<RunningState>());
 * 
 * sm.addTransition("idle", "start", "running");
 * sm.setInitialState("idle");
 * sm.start();
 * 
 * sm.triggerEvent("start"); // Transitions from idle to running
 * @endcode
 * 
 * @see State
 */
class StateMachine {
public:
    /**
     * @brief Transition information
     */
    struct Transition {
        std::string fromState;
        std::string event;
        std::string toState;
        std::function<bool(const std::any&)> guard;  // Optional guard condition
        
        Transition(const std::string& from, const std::string& evt, const std::string& to)
            : fromState(from), event(evt), toState(to) {}
    };
    
    StateMachine();
    ~StateMachine() = default;
    
    // Non-copyable
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;
    
    /**
     * @brief Add a state to the state machine
     * @param name State name/identifier
     * @param state State instance
     * @return True if added successfully, false if state already exists
     */
    bool addState(const std::string& name, StatePtr state);
    
    /**
     * @brief Remove a state from the state machine
     * @param name State name
     * @return True if removed successfully
     */
    bool removeState(const std::string& name);
    
    /**
     * @brief Add a transition between states
     * @param fromState Source state name
     * @param event Event name that triggers the transition
     * @param toState Destination state name
     * @param guard Optional guard function that must return true for transition to occur
     * @return True if transition was added successfully
     */
    bool addTransition(const std::string& fromState, const std::string& event, 
                     const std::string& toState,
                     std::function<bool(const std::any&)> guard = nullptr);
    
    /**
     * @brief Remove a transition
     * @param fromState Source state name
     * @param event Event name
     * @return True if transition was removed
     */
    bool removeTransition(const std::string& fromState, const std::string& event);
    
    /**
     * @brief Set the initial state
     * @param stateName Name of the initial state
     * @return True if state exists and was set as initial
     */
    bool setInitialState(const std::string& stateName);
    
    /**
     * @brief Start the state machine (enters initial state)
     * @return True if started successfully
     */
    bool start();
    
    /**
     * @brief Stop the state machine
     */
    void stop();
    
    /**
     * @brief Check if the state machine is running
     * @return True if running
     */
    bool isRunning() const;
    
    /**
     * @brief Trigger an event to potentially cause a state transition
     * @param eventName Name of the event
     * @param eventData Optional event data
     * @return True if transition occurred, false otherwise
     */
    bool triggerEvent(const std::string& eventName, const std::any& eventData = std::any());
    
    /**
     * @brief Get the current state name
     * @return Current state name, or empty string if not started
     */
    std::string getCurrentState() const;
    
    /**
     * @brief Get the current state instance
     * @return Current state pointer, or nullptr if not started
     */
    StatePtr getCurrentStatePtr() const;
    
    /**
     * @brief Get a state by name
     * @param name State name
     * @return State pointer, or nullptr if not found
     */
    StatePtr getState(const std::string& name) const;
    
    /**
     * @brief Update the current state (calls onUpdate)
     * @note This should be called periodically if state updates are needed
     */
    void update();
    
    /**
     * @brief Check if a transition is valid
     * @param fromState Source state
     * @param event Event name
     * @return True if transition exists and is valid
     */
    bool isValidTransition(const std::string& fromState, const std::string& event) const;
    
    /**
     * @brief Get all possible transitions from a state
     * @param stateName State name
     * @return Vector of event names that trigger transitions from this state
     */
    std::vector<std::string> getPossibleTransitions(const std::string& stateName) const;
    
    /**
     * @brief Set a callback for state transitions
     * @param callback Function called on every state transition
     */
    void setTransitionCallback(std::function<void(const std::string&, const std::string&, const std::string&)> callback);
    
    /**
     * @brief Get state history (last N states)
     * @param maxHistory Maximum number of states to return
     * @return Vector of state names in chronological order (oldest first)
     */
    std::vector<std::string> getStateHistory(size_t maxHistory = 10) const;

private:
    bool performTransition(const std::string& toState, const std::any& context = std::any());
    std::string findTransition(const std::string& fromState, const std::string& event, const std::any& eventData) const;
    
    mutable std::mutex mutex_;
    std::unordered_map<std::string, StatePtr> states_;
    std::unordered_map<std::string, std::unordered_map<std::string, Transition>> transitions_;
    std::string initialState_;
    std::string currentState_;
    std::atomic<bool> running_{false};
    std::vector<std::string> stateHistory_;
    std::function<void(const std::string&, const std::string&, const std::string&)> transitionCallback_;
    
    static constexpr size_t MAX_HISTORY = 50;
};

