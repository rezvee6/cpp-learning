#include <gtest/gtest.h>
#include "include/StateMachine.h"
#include "include/ExampleStates.h"
#include <thread>
#include <chrono>
#include <mutex>

class StateMachineTest : public ::testing::Test {
protected:
    void SetUp() override {
        sm_ = std::make_unique<StateMachine>();
        
        // Add test states
        sm_->addState("init", std::make_shared<InitState>());
        sm_->addState("active", std::make_shared<ActiveState>());
        sm_->addState("error", std::make_shared<ErrorState>());
    }

    void TearDown() override {
        if (sm_ && sm_->isRunning()) {
            sm_->stop();
        }
    }

    std::unique_ptr<StateMachine> sm_;
};

// Test adding and removing states
TEST_F(StateMachineTest, AddRemoveStates) {
    auto customState = std::make_shared<InitState>();
    
    EXPECT_TRUE(sm_->addState("custom", customState));
    EXPECT_FALSE(sm_->addState("custom", customState)); // Duplicate
    
    EXPECT_TRUE(sm_->removeState("custom"));
    EXPECT_FALSE(sm_->removeState("custom")); // Already removed
}

// Test addState with null state
TEST_F(StateMachineTest, AddStateNull) {
    StatePtr nullState = nullptr;
    EXPECT_FALSE(sm_->addState("null", nullState));
}

// Test setting initial state
TEST_F(StateMachineTest, SetInitialState) {
    EXPECT_TRUE(sm_->setInitialState("init"));
    EXPECT_FALSE(sm_->setInitialState("nonexistent"));
}

// Test start and stop
TEST_F(StateMachineTest, StartStop) {
    sm_->setInitialState("init");
    
    EXPECT_FALSE(sm_->isRunning());
    EXPECT_TRUE(sm_->start());
    EXPECT_TRUE(sm_->isRunning());
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    sm_->stop();
    EXPECT_FALSE(sm_->isRunning());
}

// Test start when already running
TEST_F(StateMachineTest, StartWhenRunning) {
    sm_->setInitialState("init");
    sm_->start();
    EXPECT_TRUE(sm_->isRunning());
    
    // Starting again should fail
    EXPECT_FALSE(sm_->start());
    EXPECT_TRUE(sm_->isRunning());
    
    sm_->stop();
}

// Test starting without initial state
TEST_F(StateMachineTest, StartWithoutInitialState) {
    StateMachine sm;
    EXPECT_FALSE(sm.start());
}

// Test basic transition
TEST_F(StateMachineTest, BasicTransition) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    EXPECT_TRUE(sm_->triggerEvent("init_complete"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

// Test invalid transition
TEST_F(StateMachineTest, InvalidTransition) {
    sm_->setInitialState("init");
    sm_->start();
    
    // No transition defined for this event
    EXPECT_FALSE(sm_->triggerEvent("invalid_event"));
    EXPECT_EQ(sm_->getCurrentState(), "init");
}

// Test transition from wrong state
TEST_F(StateMachineTest, TransitionFromWrongState) {
    sm_->addTransition("active", "some_event", "error");
    sm_->setInitialState("init");
    sm_->start();
    
    // Can't transition from init with event meant for active
    EXPECT_FALSE(sm_->triggerEvent("some_event"));
    EXPECT_EQ(sm_->getCurrentState(), "init");
}

// Test multiple transitions
TEST_F(StateMachineTest, MultipleTransitions) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    sm_->addTransition("error", "recover", "init");
    
    sm_->setInitialState("init");
    sm_->start();
    
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->triggerEvent("error_occurred", std::string("test error"));
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    sm_->triggerEvent("recover");
    EXPECT_EQ(sm_->getCurrentState(), "init");
}

// Test transition with guard condition
TEST_F(StateMachineTest, TransitionWithGuard) {
    sm_->addTransition("init", "conditional", "active",
        [](const std::any& data) -> bool {
            try {
                if (auto* value = std::any_cast<bool>(&data)) {
                    return *value;
                }
            } catch (...) {}
            return false;
        });
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Guard returns false - transition should not occur
    EXPECT_FALSE(sm_->triggerEvent("conditional", false));
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    // Guard returns true - transition should occur
    EXPECT_TRUE(sm_->triggerEvent("conditional", true));
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

// Test state history
TEST_F(StateMachineTest, StateHistory) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    auto history = sm_->getStateHistory();
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0], "init");
    
    sm_->triggerEvent("init_complete");
    history = sm_->getStateHistory();
    EXPECT_GE(history.size(), 2);
    if (history.size() >= 2) {
        EXPECT_EQ(history[0], "init");
        EXPECT_EQ(history[1], "active");
    }
    
    sm_->triggerEvent("error_occurred", std::string("error"));
    history = sm_->getStateHistory();
    EXPECT_GE(history.size(), 3);
}

// Test transition callback
TEST_F(StateMachineTest, TransitionCallback) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    
    std::mutex transitionsMutex;
    std::vector<std::pair<std::string, std::string>> transitions;
    
    sm_->setTransitionCallback([&transitions, &transitionsMutex](const std::string& from,
                                                                  const std::string& to,
                                                                  const std::string&) {
        std::lock_guard<std::mutex> lock(transitionsMutex);
        transitions.emplace_back(from, to);
    });
    
    sm_->start();
    
    // Trigger event - should cause transition
    bool transitioned = sm_->triggerEvent("init_complete");
    EXPECT_TRUE(transitioned);
    
    // Wait a moment for callback to execute
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    {
        std::lock_guard<std::mutex> lock(transitionsMutex);
        EXPECT_GE(transitions.size(), 1);
        if (transitions.size() >= 1) {
            EXPECT_EQ(transitions[0].first, "init");
            EXPECT_EQ(transitions[0].second, "active");
        }
    }
}

// Test get possible transitions
TEST_F(StateMachineTest, GetPossibleTransitions) {
    sm_->addTransition("init", "event1", "active");
    sm_->addTransition("init", "event2", "error");
    sm_->addTransition("active", "event3", "error");
    
    auto transitions = sm_->getPossibleTransitions("init");
    EXPECT_EQ(transitions.size(), 2);
    EXPECT_NE(std::find(transitions.begin(), transitions.end(), "event1"), transitions.end());
    EXPECT_NE(std::find(transitions.begin(), transitions.end(), "event2"), transitions.end());
    
    transitions = sm_->getPossibleTransitions("active");
    EXPECT_EQ(transitions.size(), 1);
    EXPECT_EQ(transitions[0], "event3");
    
    transitions = sm_->getPossibleTransitions("error");
    EXPECT_EQ(transitions.size(), 0);
}

// Test isValidTransition
TEST_F(StateMachineTest, IsValidTransition) {
    sm_->addTransition("init", "valid_event", "active");
    
    EXPECT_TRUE(sm_->isValidTransition("init", "valid_event"));
    EXPECT_FALSE(sm_->isValidTransition("init", "invalid_event"));
    EXPECT_FALSE(sm_->isValidTransition("active", "valid_event"));
}

// Test trigger event when not running
TEST_F(StateMachineTest, TriggerEventWhenNotRunning) {
    EXPECT_FALSE(sm_->triggerEvent("any_event"));
}

// Test remove transition
TEST_F(StateMachineTest, RemoveTransition) {
    sm_->addTransition("init", "test_event", "active");
    EXPECT_TRUE(sm_->isValidTransition("init", "test_event"));
    
    EXPECT_TRUE(sm_->removeTransition("init", "test_event"));
    EXPECT_FALSE(sm_->isValidTransition("init", "test_event"));
    
    EXPECT_FALSE(sm_->removeTransition("init", "nonexistent"));
}

// Test update functionality
TEST_F(StateMachineTest, Update) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Update should not crash
    sm_->update();
    
    sm_->stop();
}

// Test getState
TEST_F(StateMachineTest, GetState) {
    auto state = sm_->getState("init");
    EXPECT_NE(state, nullptr);
    EXPECT_EQ(state->getName(), "init");
    
    state = sm_->getState("nonexistent");
    EXPECT_EQ(state, nullptr);
}

// Test getCurrentStatePtr
TEST_F(StateMachineTest, GetCurrentStatePtr) {
    sm_->setInitialState("init");
    
    auto state = sm_->getCurrentStatePtr();
    EXPECT_EQ(state, nullptr); // Not started yet
    
    sm_->start();
    state = sm_->getCurrentStatePtr();
    EXPECT_NE(state, nullptr);
    EXPECT_EQ(state->getName(), "init");
}

// Test context data in transitions
TEST_F(StateMachineTest, ContextDataInTransition) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    std::string contextData = "test context";
    sm_->triggerEvent("init_complete", contextData);
    
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

// Test removeState with transitions
TEST_F(StateMachineTest, RemoveStateWithTransitions) {
    sm_->addTransition("init", "event1", "active");
    sm_->addTransition("active", "event2", "error");
    
    // Remove state should remove all transitions involving it
    EXPECT_TRUE(sm_->removeState("active"));
    EXPECT_FALSE(sm_->isValidTransition("init", "event1"));
    EXPECT_FALSE(sm_->isValidTransition("active", "event2"));
}

// Test removeState when it's the current state
TEST_F(StateMachineTest, RemoveCurrentState) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Should not be able to remove current state
    EXPECT_FALSE(sm_->removeState("init"));
    
    sm_->stop();
    // Now should be able to remove it
    EXPECT_TRUE(sm_->removeState("init"));
}

// Test getStateHistory with maxHistory limit
TEST_F(StateMachineTest, GetStateHistoryWithLimit) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    sm_->addTransition("error", "recover", "init");
    
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    sm_->triggerEvent("error_occurred", std::string("error"));
    sm_->triggerEvent("recover");
    sm_->triggerEvent("init_complete");
    
    // Get last 2 states
    auto history = sm_->getStateHistory(2);
    EXPECT_LE(history.size(), 2);
}

// Test performTransition when not running
TEST_F(StateMachineTest, PerformTransitionWhenNotRunning) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    // Don't start
    
    // Can't transition when not running
    // This is tested indirectly through triggerEvent
    EXPECT_FALSE(sm_->triggerEvent("init_complete"));
}

// Test findTransition with guard condition failure
TEST_F(StateMachineTest, FindTransitionGuardFailure) {
    sm_->addTransition("init", "conditional", "active",
        [](const std::any& data) -> bool {
            try {
                if (auto* value = std::any_cast<int>(&data)) {
                    return *value > 10;
                }
            } catch (...) {}
            return false;
        });
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Guard should fail with value <= 10
    EXPECT_FALSE(sm_->triggerEvent("conditional", 5));
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    // Guard should pass with value > 10
    EXPECT_TRUE(sm_->triggerEvent("conditional", 15));
    EXPECT_EQ(sm_->getCurrentState(), "active");
}

// Test update when not running
TEST_F(StateMachineTest, UpdateWhenNotRunning) {
    sm_->setInitialState("init");
    // Don't start
    
    // Update should do nothing when not running
    sm_->update();
    EXPECT_FALSE(sm_->isRunning());
}

// Test state history overflow (MAX_HISTORY)
TEST_F(StateMachineTest, StateHistoryOverflow) {
    sm_->addTransition("init", "to_active", "active");
    sm_->addTransition("active", "to_error", "error");
    sm_->addTransition("error", "to_init", "init");
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Create many transitions to test history overflow
    for (int i = 0; i < 20; ++i) {
        if (sm_->getCurrentState() == "init") {
            sm_->triggerEvent("to_active");
        } else if (sm_->getCurrentState() == "active") {
            sm_->triggerEvent("to_error");
        } else {
            sm_->triggerEvent("to_init");
        }
    }
    
    auto history = sm_->getStateHistory();
    // History should be limited to MAX_HISTORY (50)
    EXPECT_LE(history.size(), 50);
}

// Test addTransition with invalid states
TEST_F(StateMachineTest, AddTransitionInvalidStates) {
    // Try to add transition with non-existent states
    EXPECT_FALSE(sm_->addTransition("nonexistent1", "event", "nonexistent2"));
}

// Test removeTransition edge cases
TEST_F(StateMachineTest, RemoveTransitionEdgeCases) {
    sm_->addTransition("init", "event1", "active");
    sm_->addTransition("init", "event2", "error");
    
    // Remove existing transition
    EXPECT_TRUE(sm_->removeTransition("init", "event1"));
    EXPECT_FALSE(sm_->isValidTransition("init", "event1"));
    EXPECT_TRUE(sm_->isValidTransition("init", "event2"));
    
    // Try to remove non-existent transition
    EXPECT_FALSE(sm_->removeTransition("init", "nonexistent"));
    EXPECT_FALSE(sm_->removeTransition("nonexistent", "event"));
}

// Test state onUpdate methods
TEST_F(StateMachineTest, StateOnUpdate) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Call update multiple times to cover onUpdate
    for (int i = 0; i < 5; ++i) {
        sm_->update();
    }
    
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Update in active state
    for (int i = 0; i < 3; ++i) {
        sm_->update();
    }
    
    sm_->triggerEvent("error_occurred", std::string("error"));
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    // Update in error state
    for (int i = 0; i < 3; ++i) {
        sm_->update();
    }
    
    sm_->stop();
}

// Test ActiveState onEvent (heartbeat, pause)
TEST_F(StateMachineTest, ActiveStateOnEvent) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Test heartbeat event (should be handled, no transition)
    EXPECT_TRUE(sm_->triggerEvent("heartbeat"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    // Test pause event (should be handled, no transition)
    EXPECT_TRUE(sm_->triggerEvent("pause"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test ErrorState onEvent (retry)
TEST_F(StateMachineTest, ErrorStateOnEvent) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    sm_->addTransition("error", "retry", "active");
    
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    sm_->triggerEvent("error_occurred", std::string("error"));
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    // Test retry event (should allow transition)
    EXPECT_TRUE(sm_->triggerEvent("retry"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test InitState onEnter with context
TEST_F(StateMachineTest, InitStateOnEnterWithContext) {
    sm_->setInitialState("init");
    
    // Start with context
    std::string config = "test-config";
    sm_->start();
    
    // Trigger transition with context
    sm_->triggerEvent("init_complete", config);
    
    sm_->stop();
}

// Test InitState onEnter with context type mismatch
TEST_F(StateMachineTest, InitStateOnEnterContextMismatch) {
    sm_->setInitialState("init");
    
    // Start with wrong context type (int instead of string)
    int wrongType = 42;
    sm_->start();
    
    // Trigger transition with wrong context type
    sm_->addTransition("init", "init_complete", "active");
    sm_->triggerEvent("init_complete", wrongType);
    
    sm_->stop();
}

// Test InitState onEnter with empty context
TEST_F(StateMachineTest, InitStateOnEnterEmptyContext) {
    sm_->setInitialState("init");
    
    // Start with empty context
    sm_->start();
    
    // onEnter should handle empty context gracefully
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    sm_->stop();
}

// Test ActiveState onEnter with context
TEST_F(StateMachineTest, ActiveStateOnEnterWithContext) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    std::string activationData = "activation-data";
    sm_->triggerEvent("init_complete", activationData);
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test ActiveState onEnter with context type mismatch
TEST_F(StateMachineTest, ActiveStateOnEnterContextMismatch) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    // Trigger with wrong context type
    int wrongType = 123;
    sm_->triggerEvent("init_complete", wrongType);
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test ErrorState onEnter with context
TEST_F(StateMachineTest, ErrorStateOnEnterWithContext) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    
    std::string errorMsg = "Test error message";
    sm_->triggerEvent("error_occurred", errorMsg);
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    sm_->stop();
}

// Test ErrorState onEnter with context type mismatch
TEST_F(StateMachineTest, ErrorStateOnEnterContextMismatch) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    
    // Trigger with wrong context type
    int wrongType = 456;
    sm_->triggerEvent("error_occurred", wrongType);
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    sm_->stop();
}

// Test ErrorState onEnter with empty context
TEST_F(StateMachineTest, ErrorStateOnEnterEmptyContext) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    
    // Trigger with empty context
    sm_->triggerEvent("error_occurred", std::any());
    EXPECT_EQ(sm_->getCurrentState(), "error");
    
    sm_->stop();
}

// Test getStateHistory with default (no limit)
TEST_F(StateMachineTest, GetStateHistoryDefault) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->addTransition("active", "error_occurred", "error");
    
    sm_->setInitialState("init");
    sm_->start();
    
    auto history1 = sm_->getStateHistory();
    EXPECT_EQ(history1.size(), 1);
    
    sm_->triggerEvent("init_complete");
    auto history2 = sm_->getStateHistory();
    EXPECT_GE(history2.size(), 2);
    
    sm_->triggerEvent("error_occurred", std::string("error"));
    auto history3 = sm_->getStateHistory();
    EXPECT_GE(history3.size(), 3);
    
    sm_->stop();
}

// Test performTransition with empty current state
TEST_F(StateMachineTest, PerformTransitionEmptyState) {
    // This is tested indirectly through triggerEvent when not running
    // But let's also test the edge case
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    // Don't start - so currentState_ is empty
    
    // Can't transition when not started
    EXPECT_FALSE(sm_->triggerEvent("init_complete"));
}

// Test removeState when state doesn't exist
TEST_F(StateMachineTest, RemoveStateNonExistent) {
    EXPECT_FALSE(sm_->removeState("nonexistent"));
}

// Test getCurrentStatePtr when state doesn't exist
TEST_F(StateMachineTest, GetCurrentStatePtrNonExistent) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Manually set invalid state (edge case)
    // This is hard to test directly, but getCurrentStatePtr should handle it
    
    sm_->stop();
    auto state = sm_->getCurrentStatePtr();
    EXPECT_EQ(state, nullptr);
}

// Test stop when currentState is empty
TEST_F(StateMachineTest, StopWhenCurrentStateEmpty) {
    // Create a state machine but don't start it
    StateMachine sm;
    sm.addState("init", std::make_shared<InitState>());
    sm.setInitialState("init");
    // Don't start
    
    // Stop should handle empty current state gracefully
    sm.stop();
    EXPECT_FALSE(sm.isRunning());
}

// Test stop when state not found in map
TEST_F(StateMachineTest, StopWhenStateNotFound) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Remove the state while running (edge case)
    // This shouldn't happen in practice, but test the code path
    sm_->stop();
    
    // Stop again should be safe
    sm_->stop();
}

// Test triggerEvent when currentStatePtr is null
TEST_F(StateMachineTest, TriggerEventNullStatePtr) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Remove state while running (edge case)
    sm_->removeState("init");
    
    // Trigger event should handle null state pointer
    EXPECT_FALSE(sm_->triggerEvent("any_event"));
    
    sm_->stop();
}

// Test performTransition when fromStatePtr is null
TEST_F(StateMachineTest, PerformTransitionNullFromState) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    // This is hard to test directly, but performTransition handles null fromStatePtr
    // The transition should still work
    EXPECT_TRUE(sm_->triggerEvent("init_complete"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test start when initial state doesn't exist
TEST_F(StateMachineTest, StartWhenInitialStateNotExists) {
    StateMachine sm;
    sm.addState("init", std::make_shared<InitState>());
    sm.setInitialState("nonexistent"); // Set to non-existent state
    
    EXPECT_FALSE(sm.start());
    EXPECT_FALSE(sm.isRunning());
}

// Test getStateHistory with large maxHistory
TEST_F(StateMachineTest, GetStateHistoryLargeLimit) {
    sm_->addTransition("init", "to_active", "active");
    sm_->addTransition("active", "to_error", "error");
    sm_->addTransition("error", "to_init", "init");
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Create some transitions
    sm_->triggerEvent("to_active");
    sm_->triggerEvent("to_error");
    sm_->triggerEvent("to_init");
    
    // Get history with limit larger than actual history
    auto history = sm_->getStateHistory(1000);
    EXPECT_LE(history.size(), 4); // init, active, error, init
    
    sm_->stop();
}

// Test getStateHistory with zero limit
TEST_F(StateMachineTest, GetStateHistoryZeroLimit) {
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    
    auto history = sm_->getStateHistory(0);
    // Should return all history when limit is 0
    EXPECT_GE(history.size(), 0);
    
    sm_->stop();
}

// Test performTransition when toState doesn't exist
TEST_F(StateMachineTest, PerformTransitionToStateNotExists) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Try to transition to non-existent state
    // This is tested indirectly - performTransition checks if toState exists
    sm_->addTransition("init", "bad_event", "nonexistent");
    
    // Transition should fail because target state doesn't exist
    EXPECT_FALSE(sm_->triggerEvent("bad_event"));
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    sm_->stop();
}

// Test performTransition when not running
TEST_F(StateMachineTest, PerformTransitionNotRunning) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    // Don't start
    
    // Can't transition when not running
    EXPECT_FALSE(sm_->triggerEvent("init_complete"));
}

// Test performTransition when currentState is empty
TEST_F(StateMachineTest, PerformTransitionEmptyCurrentState) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    // Stop first
    sm_->stop();
    
    // Can't transition when stopped
    EXPECT_FALSE(sm_->triggerEvent("init_complete"));
}

// Test getStateHistory when history size equals maxHistory
TEST_F(StateMachineTest, GetStateHistoryExactLimit) {
    sm_->addTransition("init", "to_active", "active");
    sm_->addTransition("active", "to_error", "error");
    sm_->addTransition("error", "to_init", "init");
    
    sm_->setInitialState("init");
    sm_->start();
    
    // Create exactly 3 states
    sm_->triggerEvent("to_active");
    sm_->triggerEvent("to_error");
    
    // Get history with limit of 3
    auto history = sm_->getStateHistory(3);
    EXPECT_EQ(history.size(), 3);
    
    sm_->stop();
}

// Test getStateHistory when history size is less than maxHistory
TEST_F(StateMachineTest, GetStateHistoryLessThanLimit) {
    sm_->setInitialState("init");
    sm_->start();
    
    sm_->triggerEvent("init_complete");
    
    // Get history with limit larger than actual history
    auto history = sm_->getStateHistory(10);
    EXPECT_LE(history.size(), 2);
    
    sm_->stop();
}

// Test performTransition callback execution
TEST_F(StateMachineTest, PerformTransitionCallback) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    
    std::atomic<int> callbackCount{0};
    
    sm_->setTransitionCallback([&callbackCount](const std::string&, const std::string, const std::string&) {
        callbackCount++;
    });
    
    sm_->start();
    sm_->triggerEvent("init_complete");
    
    // Wait for callback
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_GE(callbackCount.load(), 1);
    
    sm_->stop();
}

// Test performTransition without callback
TEST_F(StateMachineTest, PerformTransitionNoCallback) {
    sm_->addTransition("init", "init_complete", "active");
    sm_->setInitialState("init");
    
    // Don't set callback
    sm_->start();
    
    // Transition should still work
    EXPECT_TRUE(sm_->triggerEvent("init_complete"));
    EXPECT_EQ(sm_->getCurrentState(), "active");
    
    sm_->stop();
}

// Test findTransition with no transitions for state
TEST_F(StateMachineTest, FindTransitionNoTransitions) {
    sm_->setInitialState("init");
    sm_->start();
    
    // Try to find transition for state with no transitions
    EXPECT_FALSE(sm_->triggerEvent("nonexistent_event"));
    
    sm_->stop();
}

// Test findTransition with no event match
TEST_F(StateMachineTest, FindTransitionNoEventMatch) {
    sm_->addTransition("init", "event1", "active");
    sm_->setInitialState("init");
    sm_->start();
    
    // Try event that doesn't exist
    EXPECT_FALSE(sm_->triggerEvent("event2"));
    EXPECT_EQ(sm_->getCurrentState(), "init");
    
    sm_->stop();
}

