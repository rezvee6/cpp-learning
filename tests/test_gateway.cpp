#include <gtest/gtest.h>
#include "include/MessageTypes.h"
#include "include/MessageQueue.h"
#include "include/MessageHandler.h"
#include <thread>
#include <chrono>
#include <map>
#include <string>
#include <mutex>

/**
 * @brief Test fixture for gateway data storage functionality
 * 
 * This tests the data storage and retrieval logic used by the gateway,
 * simulating the ECUDataStore behavior.
 */
class GatewayDataStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Simulate the data store structure from main.cpp
        latestData.clear();
        timestamps.clear();
    }

    void TearDown() override {
        latestData.clear();
        timestamps.clear();
    }

    // Simulated data store
    std::mutex mutex;
    std::map<std::string, std::map<std::string, std::string>> latestData;
    std::map<std::string, std::chrono::system_clock::time_point> timestamps;

    void updateData(const std::string& ecuId, const std::map<std::string, std::string>& data) {
        std::lock_guard<std::mutex> lock(mutex);
        latestData[ecuId] = data;
        timestamps[ecuId] = std::chrono::system_clock::now();
    }

    std::map<std::string, std::map<std::string, std::string>> getAllLatest() {
        std::lock_guard<std::mutex> lock(mutex);
        return latestData;
    }

    std::map<std::string, std::string> getECUData(const std::string& ecuId) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = latestData.find(ecuId);
        if (it != latestData.end()) {
            return it->second;
        }
        return {};
    }

    std::vector<std::string> getECUIds() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<std::string> ids;
        for (const auto& [id, _] : latestData) {
            ids.push_back(id);
        }
        return ids;
    }
};

// Test storing single ECU data
TEST_F(GatewayDataStoreTest, StoreSingleECUData) {
    std::map<std::string, std::string> engineData = {
        {"rpm", "2500"},
        {"temperature", "85.5"},
        {"pressure", "1.2"}
    };

    updateData("engine", engineData);

    auto retrieved = getECUData("engine");
    EXPECT_EQ(retrieved.size(), 3);
    EXPECT_EQ(retrieved["rpm"], "2500");
    EXPECT_EQ(retrieved["temperature"], "85.5");
    EXPECT_EQ(retrieved["pressure"], "1.2");
}

// Test storing multiple ECUs
TEST_F(GatewayDataStoreTest, StoreMultipleECUs) {
    std::map<std::string, std::string> engineData = {{"rpm", "3000"}};
    std::map<std::string, std::string> transData = {{"gear", "4"}};
    std::map<std::string, std::string> brakeData = {{"pressure", "50"}};

    updateData("engine", engineData);
    updateData("transmission", transData);
    updateData("brake", brakeData);

    auto allData = getAllLatest();
    EXPECT_EQ(allData.size(), 3);
    EXPECT_TRUE(allData.find("engine") != allData.end());
    EXPECT_TRUE(allData.find("transmission") != allData.end());
    EXPECT_TRUE(allData.find("brake") != allData.end());
}

// Test updating existing ECU data
TEST_F(GatewayDataStoreTest, UpdateExistingECUData) {
    std::map<std::string, std::string> initialData = {{"rpm", "2000"}};
    updateData("engine", initialData);

    std::map<std::string, std::string> updatedData = {{"rpm", "3500"}, {"temperature", "90"}};
    updateData("engine", updatedData);

    auto retrieved = getECUData("engine");
    EXPECT_EQ(retrieved.size(), 2);
    EXPECT_EQ(retrieved["rpm"], "3500");
    EXPECT_EQ(retrieved["temperature"], "90");
}

// Test retrieving non-existent ECU
TEST_F(GatewayDataStoreTest, RetrieveNonExistentECU) {
    auto data = getECUData("nonexistent");
    EXPECT_TRUE(data.empty());
}

// Test getting all ECU IDs
TEST_F(GatewayDataStoreTest, GetAllECUIds) {
    updateData("engine", {{"rpm", "2000"}});
    updateData("transmission", {{"gear", "3"}});
    updateData("battery", {{"voltage", "12.5"}});

    auto ids = getECUIds();
    EXPECT_EQ(ids.size(), 3);
    EXPECT_NE(std::find(ids.begin(), ids.end(), "engine"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "transmission"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "battery"), ids.end());
}

// Test thread safety
TEST_F(GatewayDataStoreTest, ThreadSafety) {
    const int numThreads = 4;
    const int updatesPerThread = 100;
    std::vector<std::thread> threads;

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back([this, i]() {
            for (int j = 0; j < updatesPerThread; ++j) {
                std::map<std::string, std::string> data = {
                    {"value", std::to_string(i * 1000 + j)}
                };
                updateData("ecu-" + std::to_string(i), data);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    auto allData = getAllLatest();
    EXPECT_EQ(allData.size(), numThreads);
    for (int i = 0; i < numThreads; ++i) {
        std::string ecuId = "ecu-" + std::to_string(i);
        EXPECT_TRUE(allData.find(ecuId) != allData.end());
    }
}

// Test ECUDataMessage integration with queue
TEST_F(GatewayDataStoreTest, ECUDataMessageWithQueue) {
    MessageQueue queue;
    
    std::map<std::string, std::string> data = {
        {"rpm", "2500"},
        {"temperature", "85"}
    };
    
    auto msg = std::make_shared<ECUDataMessage>("msg-1", "engine", data);
    queue.enqueue(msg);
    
    EXPECT_FALSE(queue.empty());
    EXPECT_EQ(queue.size(), 1);
    
    auto received = queue.dequeue();
    ASSERT_TRUE(received.has_value());
    
    auto ecuMsg = std::dynamic_pointer_cast<ECUDataMessage>(received.value());
    ASSERT_NE(ecuMsg, nullptr);
    EXPECT_EQ(ecuMsg->getECUId(), "engine");
    EXPECT_EQ(ecuMsg->getValue("rpm").value(), "2500");
}

// Test processing ECUDataMessage with MessageHandler
TEST_F(GatewayDataStoreTest, ProcessECUDataMessage) {
    MessageQueue queue;
    MessageHandler handler(queue, 1);
    
    std::atomic<int> processedCount{0};
    
    handler.setMessageProcessor([&processedCount](MessagePtr msg) {
        if (auto ecuMsg = std::dynamic_pointer_cast<ECUDataMessage>(msg)) {
            processedCount++;
            EXPECT_EQ(ecuMsg->getType(), "ECUDataMessage");
        }
    });
    
    handler.start();
    
    std::map<std::string, std::string> data = {{"rpm", "3000"}};
    auto msg = std::make_shared<ECUDataMessage>("msg-2", "engine", data);
    queue.enqueue(msg);
    
    // Wait for processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    handler.stop();
    
    EXPECT_GE(processedCount.load(), 1);
}

// Test multiple ECUDataMessages in queue
TEST_F(GatewayDataStoreTest, MultipleECUDataMessages) {
    MessageQueue queue;
    
    std::vector<std::string> ecuIds = {"engine", "transmission", "brake", "battery"};
    
    for (size_t i = 0; i < ecuIds.size(); ++i) {
        std::map<std::string, std::string> data = {
            {"value", std::to_string(i)}
        };
        auto msg = std::make_shared<ECUDataMessage>(
            "msg-" + std::to_string(i), 
            ecuIds[i], 
            data
        );
        queue.enqueue(msg);
    }
    
    EXPECT_EQ(queue.size(), ecuIds.size());
    
    for (size_t i = 0; i < ecuIds.size(); ++i) {
        auto received = queue.dequeue();
        ASSERT_TRUE(received.has_value());
        auto ecuMsg = std::dynamic_pointer_cast<ECUDataMessage>(received.value());
        ASSERT_NE(ecuMsg, nullptr);
        EXPECT_EQ(ecuMsg->getECUId(), ecuIds[i]);
    }
}

// Test JSON-like string parsing (simplified version)
TEST_F(GatewayDataStoreTest, SimpleJSONParsing) {
    // Test extracting ECU ID from JSON-like string
    std::string jsonStr = R"({"id":"test-123","ecuId":"engine","data":{"rpm":"2500"}})";
    
    // Simple extraction test
    size_t pos = jsonStr.find("\"ecuId\":\"");
    ASSERT_NE(pos, std::string::npos);
    pos += 9; // Skip "ecuId":"
    size_t end = jsonStr.find("\"", pos);
    ASSERT_NE(end, std::string::npos);
    std::string ecuId = jsonStr.substr(pos, end - pos);
    
    EXPECT_EQ(ecuId, "engine");
}

// Test empty data handling
TEST_F(GatewayDataStoreTest, EmptyDataHandling) {
    std::map<std::string, std::string> emptyData;
    updateData("test", emptyData);
    
    auto retrieved = getECUData("test");
    EXPECT_TRUE(retrieved.empty());
    
    auto ids = getECUIds();
    EXPECT_EQ(ids.size(), 1);
    EXPECT_EQ(ids[0], "test");
}

