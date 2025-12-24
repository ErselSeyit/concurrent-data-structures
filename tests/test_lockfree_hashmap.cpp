#include <gtest/gtest.h>
#include "concurrent/lockfree_hashmap.hpp"
#include <thread>
#include <vector>
#include <string>

using namespace concurrent;

class LockFreeHashMapTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LockFreeHashMapTest, BasicInsertGet) {
    LockFreeHashMap<int, int> map;
    
    map.insert(1, 100);
    auto result = map.get(1);
    
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 100);
}

TEST_F(LockFreeHashMapTest, NonExistentKey) {
    LockFreeHashMap<int, int> map;
    
    auto result = map.get(999);
    ASSERT_FALSE(result.has_value());
}

TEST_F(LockFreeHashMapTest, UpdateValue) {
    LockFreeHashMap<int, int> map;
    
    map.insert(1, 100);
    map.insert(1, 200);
    
    auto result = map.get(1);
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 200);
}

TEST_F(LockFreeHashMapTest, Erase) {
    LockFreeHashMap<int, int> map;
    
    map.insert(1, 100);
    ASSERT_TRUE(map.contains(1));
    
    ASSERT_TRUE(map.erase(1));
    ASSERT_FALSE(map.contains(1));
    ASSERT_FALSE(map.erase(1)); // Erasing non-existent key
}

TEST_F(LockFreeHashMapTest, MultipleKeys) {
    LockFreeHashMap<int, std::string> map;
    
    for (int i = 0; i < 1000; ++i) {
        map.insert(i, "value_" + std::to_string(i));
    }
    
    for (int i = 0; i < 1000; ++i) {
        auto result = map.get(i);
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), "value_" + std::to_string(i));
    }
    
    ASSERT_EQ(map.size(), 1000);
}

TEST_F(LockFreeHashMapTest, ConcurrentInsert) {
    LockFreeHashMap<int, int> map;
    constexpr int num_threads = 8;
    constexpr int items_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&map, t]() {
            for (int i = 0; i < items_per_thread; ++i) {
                int key = t * items_per_thread + i;
                map.insert(key, key * 2);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all items
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < items_per_thread; ++i) {
            int key = t * items_per_thread + i;
            auto result = map.get(key);
            ASSERT_TRUE(result.has_value());
            ASSERT_EQ(result.value(), key * 2);
        }
    }
    
    ASSERT_EQ(map.size(), num_threads * items_per_thread);
}

TEST_F(LockFreeHashMapTest, ConcurrentReadWrite) {
    LockFreeHashMap<int, int> map;
    constexpr int num_writers = 4;
    constexpr int num_readers = 4;
    constexpr int items_per_writer = 500;
    
    std::vector<std::thread> threads;
    
    // Writers
    for (int t = 0; t < num_writers; ++t) {
        threads.emplace_back([&map, t]() {
            for (int i = 0; i < items_per_writer; ++i) {
                int key = t * items_per_writer + i;
                map.insert(key, key * 2);
            }
        });
    }
    
    // Readers
    for (int t = 0; t < num_readers; ++t) {
        threads.emplace_back([&map]() {
            for (int i = 0; i < 10000; ++i) {
                map.get(i % (num_writers * items_per_writer));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(LockFreeHashMapTest, EmptyAndSize) {
    LockFreeHashMap<int, int> map;
    
    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0);
    
    map.insert(1, 100);
    ASSERT_FALSE(map.empty());
    ASSERT_EQ(map.size(), 1);
    
    map.erase(1);
    ASSERT_TRUE(map.empty());
    ASSERT_EQ(map.size(), 0);
}

