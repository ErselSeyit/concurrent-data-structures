#include <gtest/gtest.h>
#include "concurrent/thread_pool.hpp"
#include <chrono>
#include <vector>
#include <atomic>

using namespace concurrent;

class ThreadPoolTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ThreadPoolTest, BasicTaskExecution) {
    ThreadPool pool(4);
    
    auto future = pool.submit([]() {
        return 42;
    });
    
    ASSERT_EQ(future.get(), 42);
}

TEST_F(ThreadPoolTest, MultipleTasks) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([i]() {
            return i * 2;
        }));
    }
    
    for (size_t i = 0; i < futures.size(); ++i) {
        ASSERT_EQ(futures[i].get(), static_cast<int>(i * 2));
    }
}

TEST_F(ThreadPoolTest, ParallelSum) {
    ThreadPool pool(4);
    constexpr int num_tasks = 1000;
    
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < num_tasks; ++i) {
        futures.push_back(pool.submit([i]() {
            int sum = 0;
            for (int j = 0; j < 100; ++j) {
                sum += i + j;
            }
            return sum;
        }));
    }
    
    int total_sum = 0;
    for (auto& future : futures) {
        total_sum += future.get();
    }
    
    ASSERT_GT(total_sum, 0);
}

TEST_F(ThreadPoolTest, WaitForCompletion) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    
    for (int i = 0; i < 100; ++i) {
        pool.submit([&counter]() {
            counter.fetch_add(1);
        });
    }
    
    pool.wait();
    
    ASSERT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, TaskWithArguments) {
    ThreadPool pool(4);
    
    auto add = [](int a, int b) {
        return a + b;
    };
    
    auto future = pool.submit(add, 10, 20);
    ASSERT_EQ(future.get(), 30);
}

TEST_F(ThreadPoolTest, VoidReturnType) {
    ThreadPool pool(4);
    std::atomic<bool> executed{false};
    
    auto future = pool.submit([&executed]() {
        executed.store(true);
    });
    
    future.wait();
    ASSERT_TRUE(executed.load());
}

