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

// Test ECUDataMessage creation and basic properties
TEST_F(MessageTypesTest, ECUDataMessageCreation) {
    std::map<std::string, std::string> data = {
        {"rpm", "2500"},
        {"temperature", "85.5"},
        {"pressure", "1.2"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("ecu-1", "engine", data);
    
    EXPECT_EQ(msg->getType(), "ECUDataMessage");
    EXPECT_EQ(msg->getId(), "ecu-1");
    EXPECT_EQ(msg->getECUId(), "engine");
    EXPECT_NE(msg->getTimestamp().time_since_epoch().count(), 0);
}

// Test ECUDataMessage getData
TEST_F(MessageTypesTest, ECUDataMessageGetData) {
    std::map<std::string, std::string> data = {
        {"rpm", "3000"},
        {"temperature", "90"},
        {"throttle_position", "45.5"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("ecu-2", "engine", data);
    const auto& retrievedData = msg->getData();
    
    EXPECT_EQ(retrievedData.size(), 3);
    EXPECT_EQ(retrievedData.at("rpm"), "3000");
    EXPECT_EQ(retrievedData.at("temperature"), "90");
    EXPECT_EQ(retrievedData.at("throttle_position"), "45.5");
}

// Test ECUDataMessage getValue
TEST_F(MessageTypesTest, ECUDataMessageGetValue) {
    std::map<std::string, std::string> data = {
        {"gear", "3"},
        {"speed", "60.5"},
        {"temperature", "75"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("ecu-3", "transmission", data);
    
    auto gear = msg->getValue("gear");
    EXPECT_TRUE(gear.has_value());
    EXPECT_EQ(gear.value(), "3");
    
    auto speed = msg->getValue("speed");
    EXPECT_TRUE(speed.has_value());
    EXPECT_EQ(speed.value(), "60.5");
    
    auto missing = msg->getValue("missing_key");
    EXPECT_FALSE(missing.has_value());
}

// Test ECUDataMessage toString
TEST_F(MessageTypesTest, ECUDataMessageToString) {
    std::map<std::string, std::string> data = {
        {"voltage", "12.5"},
        {"current", "2.3"},
        {"state_of_charge", "85.0"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("ecu-4", "battery", data);
    std::string str = msg->toString();
    
    EXPECT_NE(str.find("ECUDataMessage"), std::string::npos);
    EXPECT_NE(str.find("ecu-4"), std::string::npos);
    EXPECT_NE(str.find("battery"), std::string::npos);
    EXPECT_NE(str.find("voltage"), std::string::npos);
    EXPECT_NE(str.find("12.5"), std::string::npos);
}

// Test ECUDataMessage process
TEST_F(MessageTypesTest, ECUDataMessageProcess) {
    std::map<std::string, std::string> data = {
        {"brake_pressure", "50.0"},
        {"abs_active", "true"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("ecu-5", "brake", data);
    // Should not crash
    msg->process();
}

// Test ECUDataMessage with empty data
TEST_F(MessageTypesTest, ECUDataMessageEmptyData) {
    std::map<std::string, std::string> emptyData;
    auto msg = std::make_shared<ECUDataMessage>("ecu-6", "test", emptyData);
    
    EXPECT_EQ(msg->getECUId(), "test");
    EXPECT_TRUE(msg->getData().empty());
    EXPECT_FALSE(msg->getValue("any_key").has_value());
}

// Test ECUDataMessage timestamp
TEST_F(MessageTypesTest, ECUDataMessageTimestamp) {
    std::map<std::string, std::string> data = {{"test", "value"}};
    auto msg = std::make_shared<ECUDataMessage>("ecu-7", "test", data);
    
    auto timestamp = msg->getTimestamp();
    auto now = std::chrono::system_clock::now();
    auto oneMinute = std::chrono::minutes(1);
    
    EXPECT_LT(now - timestamp, oneMinute);
    EXPECT_GT(timestamp.time_since_epoch().count(), 0);
}

// Test ECUDataMessage with multiple ECUs
TEST_F(MessageTypesTest, ECUDataMessageMultipleECUs) {
    std::map<std::string, std::string> engineData = {{"rpm", "2000"}};
    std::map<std::string, std::string> transData = {{"gear", "4"}};
    std::map<std::string, std::string> brakeData = {{"pressure", "30"}};
    
    auto engineMsg = std::make_shared<ECUDataMessage>("id-1", "engine", engineData);
    auto transMsg = std::make_shared<ECUDataMessage>("id-2", "transmission", transData);
    auto brakeMsg = std::make_shared<ECUDataMessage>("id-3", "brake", brakeData);
    
    EXPECT_EQ(engineMsg->getECUId(), "engine");
    EXPECT_EQ(transMsg->getECUId(), "transmission");
    EXPECT_EQ(brakeMsg->getECUId(), "brake");
    
    EXPECT_EQ(engineMsg->getValue("rpm").value(), "2000");
    EXPECT_EQ(transMsg->getValue("gear").value(), "4");
    EXPECT_EQ(brakeMsg->getValue("pressure").value(), "30");
}

