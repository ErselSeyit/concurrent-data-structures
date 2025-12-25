#include <iostream>
#include <thread>
#include <vector>
#include "concurrent/lockfree_queue.hpp"
#include "concurrent/lockfree_hashmap.hpp"
#include "concurrent/thread_pool.hpp"

using namespace concurrent;

void example_queue() {
    std::cout << "\n=== Lock-Free Queue Example ===" << std::endl;
    
    LockFreeQueue<int> queue;
    
    // Producer thread
    std::thread producer([&queue]() {
        for (int i = 0; i < 10; ++i) {
            queue.enqueue(i);
            std::cout << "Produced: " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    
    // Consumer thread
    std::thread consumer([&queue]() {
        int consumed = 0;
        while (consumed < 10) {
            auto item = queue.dequeue();
            if (item.has_value()) {
                std::cout << "Consumed: " << item.value() << std::endl;
                ++consumed;
            }
        }
    });
    
    producer.join();
    consumer.join();
}

void example_hashmap() {
    std::cout << "\n=== Lock-Free Hash Map Example ===" << std::endl;
    
    LockFreeHashMap<std::string, int> map;
    
    // Insert some values
    map.insert("apple", 5);
    map.insert("banana", 3);
    map.insert("cherry", 8);
    
    // Retrieve values
    if (auto count = map.get("apple")) {
        std::cout << "Apples: " << count.value() << std::endl;
    }
    
    if (auto count = map.get("banana")) {
        std::cout << "Bananas: " << count.value() << std::endl;
    }
    
    // Update value
    map.insert("apple", 10);
    if (auto count = map.get("apple")) {
        std::cout << "Updated apples: " << count.value() << std::endl;
    }
    
    // Check existence
    std::cout << "Contains 'cherry': " << map.contains("cherry") << std::endl;
    std::cout << "Map size: " << map.size() << std::endl;
}

void example_thread_pool() {
    std::cout << "\n=== Thread Pool Example ===" << std::endl;
    
    ThreadPool pool(4);
    
    // Submit multiple tasks
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            // Simulate some computation
            int result = 0;
            for (int j = 0; j < 1000; ++j) {
                result += i + j;
            }
            return result;
        }));
    }
    
    // Wait for results
    int total = 0;
    for (size_t i = 0; i < futures.size(); ++i) {
        int result = futures[i].get();
        std::cout << "Task " << i << " result: " << result << std::endl;
        total += result;
    }
    
    std::cout << "Total: " << total << std::endl;
    std::cout << "Active tasks: " << pool.active_tasks() << std::endl;
    
    // Explicitly wait for all tasks before pool destruction
    pool.wait();
}

int main() {
    std::cout << "Concurrent Data Structures - Examples\n";
    std::cout << "=====================================\n";
    
    example_queue();
    example_hashmap();
    example_thread_pool();
    
    std::cout << "\nAll examples completed!" << std::endl;
    return 0;
}

