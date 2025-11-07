#include <gtest/gtest.h>
#include "include/MessageTypes.h"
#include <sstream>

class MessageTypesTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// Test DataMessage creation and basic properties
TEST_F(MessageTypesTest, DataMessageCreation) {
    auto msg = std::make_shared<DataMessage>("test-id", "test data");
    
    EXPECT_EQ(msg->getType(), "DataMessage");
    EXPECT_EQ(msg->getId(), "test-id");
    EXPECT_EQ(msg->getData(), "test data");
    EXPECT_NE(msg->getTimestamp().time_since_epoch().count(), 0);
}

// Test DataMessage toString
TEST_F(MessageTypesTest, DataMessageToString) {
    auto msg = std::make_shared<DataMessage>("test-id", "test data");
    std::string str = msg->toString();
    
    EXPECT_NE(str.find("DataMessage"), std::string::npos);
    EXPECT_NE(str.find("test-id"), std::string::npos);
    EXPECT_NE(str.find("test data"), std::string::npos);
}

// Test DataMessage process
TEST_F(MessageTypesTest, DataMessageProcess) {
    auto msg = std::make_shared<DataMessage>("test-id", "test data");
    // Should not crash
    msg->process();
}

// Test EventMessage creation with different types
TEST_F(MessageTypesTest, EventMessageCreation) {
    auto infoMsg = std::make_shared<EventMessage>(
        "info-1", EventMessage::EventType::INFO, "Info message"
    );
    EXPECT_EQ(infoMsg->getType(), "EventMessage");
    EXPECT_EQ(infoMsg->getId(), "info-1");
    EXPECT_EQ(infoMsg->getEventType(), EventMessage::EventType::INFO);
    EXPECT_EQ(infoMsg->getDescription(), "Info message");
    
    auto warningMsg = std::make_shared<EventMessage>(
        "warn-1", EventMessage::EventType::WARNING, "Warning message"
    );
    EXPECT_EQ(warningMsg->getEventType(), EventMessage::EventType::WARNING);
    
    auto errorMsg = std::make_shared<EventMessage>(
        "error-1", EventMessage::EventType::ERROR, "Error message"
    );
    EXPECT_EQ(errorMsg->getEventType(), EventMessage::EventType::ERROR);
}

// Test EventMessage toString for all types
TEST_F(MessageTypesTest, EventMessageToString) {
    auto infoMsg = std::make_shared<EventMessage>(
        "info-1", EventMessage::EventType::INFO, "Info message"
    );
    std::string infoStr = infoMsg->toString();
    EXPECT_NE(infoStr.find("EventMessage"), std::string::npos);
    EXPECT_NE(infoStr.find("INFO"), std::string::npos);
    EXPECT_NE(infoStr.find("Info message"), std::string::npos);
    
    auto warningMsg = std::make_shared<EventMessage>(
        "warn-1", EventMessage::EventType::WARNING, "Warning message"
    );
    std::string warnStr = warningMsg->toString();
    EXPECT_NE(warnStr.find("WARNING"), std::string::npos);
    
    auto errorMsg = std::make_shared<EventMessage>(
        "error-1", EventMessage::EventType::ERROR, "Error message"
    );
    std::string errorStr = errorMsg->toString();
    EXPECT_NE(errorStr.find("ERROR"), std::string::npos);
}

// Test EventMessage process
TEST_F(MessageTypesTest, EventMessageProcess) {
    auto msg = std::make_shared<EventMessage>(
        "event-1", EventMessage::EventType::INFO, "Test event"
    );
    // Should not crash
    msg->process();
}

// Test message timestamps
TEST_F(MessageTypesTest, MessageTimestamps) {
    auto dataMsg = std::make_shared<DataMessage>("id-1", "data");
    auto eventMsg = std::make_shared<EventMessage>(
        "id-2", EventMessage::EventType::INFO, "event"
    );
    
    auto dataTime = dataMsg->getTimestamp();
    auto eventTime = eventMsg->getTimestamp();
    
    // Timestamps should be recent (within last minute)
    auto now = std::chrono::system_clock::now();
    auto oneMinute = std::chrono::minutes(1);
    
    EXPECT_LT(now - dataTime, oneMinute);
    EXPECT_LT(now - eventTime, oneMinute);
}

// Test DataMessage getData
TEST_F(MessageTypesTest, DataMessageGetData) {
    auto msg = std::make_shared<DataMessage>("test-id", "test data payload");
    EXPECT_EQ(msg->getData(), "test data payload");
}

// Test EventMessage getDescription
TEST_F(MessageTypesTest, EventMessageGetDescription) {
    auto msg = std::make_shared<EventMessage>(
        "event-1", EventMessage::EventType::ERROR, "Database connection failed"
    );
    EXPECT_EQ(msg->getDescription(), "Database connection failed");
}

// Test EventMessage all event types
TEST_F(MessageTypesTest, EventMessageAllTypes) {
    auto info = std::make_shared<EventMessage>("1", EventMessage::EventType::INFO, "info");
    auto warning = std::make_shared<EventMessage>("2", EventMessage::EventType::WARNING, "warning");
    auto error = std::make_shared<EventMessage>("3", EventMessage::EventType::ERROR, "error");
    
    EXPECT_EQ(info->getEventType(), EventMessage::EventType::INFO);
    EXPECT_EQ(warning->getEventType(), EventMessage::EventType::WARNING);
    EXPECT_EQ(error->getEventType(), EventMessage::EventType::ERROR);
}

