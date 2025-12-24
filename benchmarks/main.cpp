#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include "concurrent/lockfree_queue.hpp"
#include "concurrent/lockfree_hashmap.hpp"
#include "concurrent/thread_pool.hpp"

using namespace concurrent;
using namespace std::chrono;

template<typename Func>
double benchmark(Func&& func, const std::string& name, int iterations = 1) {
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; ++i) {
        func();
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start).count();
    double avg_time = duration / static_cast<double>(iterations);
    std::cout << name << ": " << avg_time << " μs" << std::endl;
    return avg_time;
}

void benchmark_queue() {
    std::cout << "\n=== Lock-Free Queue Benchmarks ===" << std::endl;
    
    constexpr int num_operations = 1000000;
    constexpr int num_threads = 8;
    
    LockFreeQueue<int> queue;
    
    // Single-threaded enqueue/dequeue
    benchmark([&]() {
        for (int i = 0; i < num_operations; ++i) {
            queue.enqueue(i);
        }
        for (int i = 0; i < num_operations; ++i) {
            queue.dequeue();
        }
    }, "Single-threaded (1M ops)", 1);
    
    // Multi-threaded producer-consumer
    benchmark([&]() {
        LockFreeQueue<int> q;
        std::vector<std::thread> threads;
        
        // Producers
        for (int t = 0; t < num_threads / 2; ++t) {
            threads.emplace_back([&q, num_operations, t]() {
                for (int i = 0; i < num_operations / (num_threads / 2); ++i) {
                    q.enqueue(i + t * 1000000);
                }
            });
        }
        
        // Consumers
        std::atomic<int> consumed{0};
        for (int t = 0; t < num_threads / 2; ++t) {
            threads.emplace_back([&q, &consumed, num_operations]() {
                while (consumed.load() < num_operations) {
                    if (q.dequeue().has_value()) {
                        consumed.fetch_add(1);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }, "Multi-threaded producer-consumer (8 threads)", 1);
}

void benchmark_hashmap() {
    std::cout << "\n=== Lock-Free Hash Map Benchmarks ===" << std::endl;
    
    constexpr int num_operations = 100000;
    constexpr int num_threads = 8;
    
    LockFreeHashMap<int, int> map;
    
    // Single-threaded insert/lookup
    benchmark([&]() {
        for (int i = 0; i < num_operations; ++i) {
            map.insert(i, i * 2);
        }
        for (int i = 0; i < num_operations; ++i) {
            map.get(i);
        }
    }, "Single-threaded insert/lookup (100K ops)", 1);
    
    // Multi-threaded concurrent operations
    benchmark([&]() {
        LockFreeHashMap<int, int> m;
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&m, num_operations, t]() {
                for (int i = 0; i < num_operations / num_threads; ++i) {
                    int key = i + t * 10000;
                    m.insert(key, key * 2);
                    m.get(key);
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
    }, "Multi-threaded concurrent ops (8 threads)", 1);
}

void benchmark_thread_pool() {
    std::cout << "\n=== Thread Pool Benchmarks ===" << std::endl;
    
    constexpr int num_tasks = 10000;
    
    ThreadPool pool;
    
    // Parallel task execution
    benchmark([&]() {
        std::vector<std::future<int>> futures;
        futures.reserve(num_tasks);
        
        for (int i = 0; i < num_tasks; ++i) {
            futures.push_back(pool.submit([i]() {
                // Simulate some work
                int sum = 0;
                for (int j = 0; j < 1000; ++j) {
                    sum += i + j;
                }
                return sum;
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
        
        pool.wait();
    }, "Thread pool (10K tasks)", 1);
}

int main() {
    std::cout << "High-Performance Concurrent Data Structures Benchmarks\n";
    std::cout << "=====================================================\n";
    
    benchmark_queue();
    benchmark_hashmap();
    benchmark_thread_pool();
    
    std::cout << "\nBenchmarks completed!" << std::endl;
    return 0;
}

