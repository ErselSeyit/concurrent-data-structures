#include <gtest/gtest.h>
#include "concurrent/lockfree_queue.hpp"
#include <thread>
#include <vector>
#include <atomic>

using namespace concurrent;

class LockFreeQueueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(LockFreeQueueTest, BasicEnqueueDequeue) {
    LockFreeQueue<int> queue;
    
    ASSERT_TRUE(queue.enqueue(42));
    auto result = queue.dequeue();
    
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(result.value(), 42);
}

TEST_F(LockFreeQueueTest, EmptyQueue) {
    LockFreeQueue<int> queue;
    
    ASSERT_TRUE(queue.empty());
    auto result = queue.dequeue();
    ASSERT_FALSE(result.has_value());
}

TEST_F(LockFreeQueueTest, MultipleElements) {
    LockFreeQueue<int> queue;
    
    for (int i = 0; i < 100; ++i) {
        queue.enqueue(i);
    }
    
    for (int i = 0; i < 100; ++i) {
        auto result = queue.dequeue();
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), i);
    }
    
    ASSERT_TRUE(queue.empty());
}

TEST_F(LockFreeQueueTest, ConcurrentEnqueue) {
    LockFreeQueue<int> queue;
    constexpr int num_threads = 8;
    constexpr int items_per_thread = 1000;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&queue, t]() {
            for (int i = 0; i < items_per_thread; ++i) {
                queue.enqueue(t * items_per_thread + i);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all items are present
    std::vector<bool> found(num_threads * items_per_thread, false);
    int count = 0;
    
    while (!queue.empty()) {
        auto result = queue.dequeue();
        if (result.has_value()) {
            int value = result.value();
            ASSERT_GE(value, 0);
            ASSERT_LT(value, num_threads * items_per_thread);
            found[value] = true;
            ++count;
        }
    }
    
    ASSERT_EQ(count, num_threads * items_per_thread);
    for (bool f : found) {
        ASSERT_TRUE(f);
    }
}

TEST_F(LockFreeQueueTest, ConcurrentProducerConsumer) {
    LockFreeQueue<int> queue;
    constexpr int num_producers = 4;
    constexpr int num_consumers = 4;
    constexpr int items_per_producer = 1000;
    
    std::atomic<int> consumed{0};
    std::vector<std::thread> threads;
    
    // Producers
    for (int t = 0; t < num_producers; ++t) {
        threads.emplace_back([&queue, t]() {
            for (int i = 0; i < items_per_producer; ++i) {
                queue.enqueue(t * items_per_producer + i);
            }
        });
    }
    
    // Consumers
    for (int t = 0; t < num_consumers; ++t) {
        threads.emplace_back([&queue, &consumed]() {
            while (consumed.load() < num_producers * items_per_producer) {
                auto result = queue.dequeue();
                if (result.has_value()) {
                    consumed.fetch_add(1);
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    ASSERT_EQ(consumed.load(), num_producers * items_per_producer);
}

TEST_F(LockFreeQueueTest, MoveSemantics) {
    LockFreeQueue<std::unique_ptr<int>> queue;
    
    auto ptr = std::make_unique<int>(42);
    queue.enqueue(std::move(ptr));
    
    auto result = queue.dequeue();
    ASSERT_TRUE(result.has_value());
    ASSERT_EQ(*result.value(), 42);
}

