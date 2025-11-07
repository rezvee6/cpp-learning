#include "include/StateMachine.h"
#include <algorithm>

StateMachine::StateMachine() {
    stateHistory_.reserve(MAX_HISTORY);
}

bool StateMachine::addState(const std::string& name, StatePtr state) {
    if (!state) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (states_.find(name) != states_.end()) {
        return false; // State already exists
    }
    
    states_[name] = state;
    return true;
}

bool StateMachine::removeState(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Don't allow removal of current state
    if (currentState_ == name && running_.load()) {
        return false;
    }
    
    // Remove state
    auto it = states_.find(name);
    if (it == states_.end()) {
        return false;
    }
    
    states_.erase(it);
    
    // Remove all transitions involving this state
    transitions_.erase(name);
    for (auto& [fromState, eventMap] : transitions_) {
        auto eventIt = eventMap.begin();
        while (eventIt != eventMap.end()) {
            if (eventIt->second.toState == name) {
                eventIt = eventMap.erase(eventIt);
            } else {
                ++eventIt;
            }
        }
    }
    
    return true;
}

bool StateMachine::addTransition(const std::string& fromState, const std::string& event,
                                const std::string& toState,
                                std::function<bool(const std::any&)> guard) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Verify states exist
    if (states_.find(fromState) == states_.end() || 
        states_.find(toState) == states_.end()) {
        return false;
    }
    
    Transition transition(fromState, event, toState);
    transition.guard = guard;
    
    // Use emplace to avoid default construction requirement
    transitions_[fromState].emplace(event, transition);
    return true;
}

bool StateMachine::removeTransition(const std::string& fromState, const std::string& event) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto stateIt = transitions_.find(fromState);
    if (stateIt == transitions_.end()) {
        return false;
    }
    
    auto eventIt = stateIt->second.find(event);
    if (eventIt == stateIt->second.end()) {
        return false;
    }
    
    stateIt->second.erase(eventIt);
    return true;
}

bool StateMachine::setInitialState(const std::string& stateName) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (states_.find(stateName) == states_.end()) {
        return false;
    }
    
    initialState_ = stateName;
    return true;
}

bool StateMachine::start() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (running_.load()) {
        return false; // Already running
    }
    
    if (initialState_.empty()) {
        return false; // No initial state set
    }
    
    auto stateIt = states_.find(initialState_);
    if (stateIt == states_.end()) {
        return false; // Initial state doesn't exist
    }
    
    running_.store(true);
    currentState_ = initialState_;
    stateHistory_.clear();
    stateHistory_.push_back(initialState_);
    
    // Get state pointer while holding lock, then unlock before calling onEnter
    StatePtr initialStatePtr = stateIt->second;
    lock.unlock();
    
    if (initialStatePtr) {
        initialStatePtr->onEnter(std::any(), *this);
    }
    
    return true;
}

void StateMachine::stop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!running_.load()) {
        return;
    }
    
    // Get state pointer while holding lock, then unlock before calling onExit
    StatePtr currentStatePtr = nullptr;
    if (!currentState_.empty()) {
        auto stateIt = states_.find(currentState_);
        if (stateIt != states_.end()) {
            currentStatePtr = stateIt->second;
        }
    }
    
    lock.unlock();
    
    if (currentStatePtr) {
        currentStatePtr->onExit(*this);
    }
    
    lock.lock();
    running_.store(false);
    currentState_.clear();
}

bool StateMachine::isRunning() const {
    return running_.load();
}

bool StateMachine::triggerEvent(const std::string& eventName, const std::any& eventData) {
    if (!running_.load()) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (currentState_.empty()) {
        return false;
    }
    
    // Get state pointer while holding lock (avoid deadlock)
    StatePtr currentStatePtr = nullptr;
    auto stateIt = states_.find(currentState_);
    if (stateIt != states_.end()) {
        currentStatePtr = stateIt->second;
    }
    
    // Find transition while holding lock
    std::string targetState = findTransition(currentState_, eventName, eventData);
    
    // Unlock before calling state methods
    lock.unlock();
    
    // First, let the current state handle the event
    bool handled = false;
    if (currentStatePtr) {
        handled = currentStatePtr->onEvent(eventName, eventData, *this);
    }
    
    // If state handled the event, don't try to transition
    if (handled) {
        return true;
    }
    
    // Perform transition if valid
    if (!targetState.empty()) {
        return performTransition(targetState, eventData);
    }
    
    return false; // No valid transition
}

std::string StateMachine::getCurrentState() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return currentState_;
}

StatePtr StateMachine::getCurrentStatePtr() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentState_.empty()) {
        return nullptr;
    }
    return getState(currentState_);
}

StatePtr StateMachine::getState(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = states_.find(name);
    if (it != states_.end()) {
        return it->second;
    }
    return nullptr;
}

void StateMachine::update() {
    if (!running_.load()) {
        return;
    }
    
    auto currentStatePtr = getCurrentStatePtr();
    if (currentStatePtr) {
        currentStatePtr->onUpdate(*this);
    }
}

bool StateMachine::isValidTransition(const std::string& fromState, const std::string& event) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto stateIt = transitions_.find(fromState);
    if (stateIt == transitions_.end()) {
        return false;
    }
    
    return stateIt->second.find(event) != stateIt->second.end();
}

std::vector<std::string> StateMachine::getPossibleTransitions(const std::string& stateName) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> events;
    auto stateIt = transitions_.find(stateName);
    if (stateIt != transitions_.end()) {
        for (const auto& [event, transition] : stateIt->second) {
            events.push_back(event);
        }
    }
    
    return events;
}

void StateMachine::setTransitionCallback(
    std::function<void(const std::string&, const std::string&, const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    transitionCallback_ = std::move(callback);
}

std::vector<std::string> StateMachine::getStateHistory(size_t maxHistory) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t start = stateHistory_.size() > maxHistory ? 
                   stateHistory_.size() - maxHistory : 0;
    
    return std::vector<std::string>(
        stateHistory_.begin() + start,
        stateHistory_.end()
    );
}

bool StateMachine::performTransition(const std::string& toState, const std::any& context) {
    std::unique_lock<std::mutex> lock(mutex_);
    
    if (!running_.load() || currentState_.empty()) {
        return false;
    }
    
    auto toStateIt = states_.find(toState);
    if (toStateIt == states_.end()) {
        return false;
    }
    
    std::string fromState = currentState_;
    
    // Get state pointers while holding lock (avoid deadlock)
    StatePtr fromStatePtr = nullptr;
    auto fromStateIt = states_.find(fromState);
    if (fromStateIt != states_.end()) {
        fromStatePtr = fromStateIt->second;
    }
    
    StatePtr toStatePtr = toStateIt->second;
    
    // Update current state and history while holding lock
    currentState_ = toState;
    stateHistory_.push_back(toState);
    if (stateHistory_.size() > MAX_HISTORY) {
        stateHistory_.erase(stateHistory_.begin());
    }
    
    // Store callback locally
    auto callback = transitionCallback_;
    
    // Unlock before calling state methods
    lock.unlock();
    
    // Call onExit for current state
    if (fromStatePtr) {
        fromStatePtr->onExit(*this);
    }
    
    // Call transition callback
    if (callback) {
        callback(fromState, toState, "");
    }
    
    // Call onEnter for new state
    if (toStatePtr) {
        toStatePtr->onEnter(context, *this);
    }
    
    return true;
}

std::string StateMachine::findTransition(const std::string& fromState, 
                                        const std::string& event,
                                        const std::any& eventData) const {
    auto stateIt = transitions_.find(fromState);
    if (stateIt == transitions_.end()) {
        return "";
    }
    
    auto eventIt = stateIt->second.find(event);
    if (eventIt == stateIt->second.end()) {
        return "";
    }
    
    const Transition& transition = eventIt->second;
    
    // Check guard condition if present
    if (transition.guard) {
        if (!transition.guard(eventData)) {
            return ""; // Guard condition failed
        }
    }
    
    return transition.toState;
}

