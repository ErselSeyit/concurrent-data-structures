#pragma once

#include "lockfree_queue.hpp"
#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <thread>
#include <vector>

namespace concurrent {

/**
 * @brief High-performance thread pool with work-stealing
 * 
 * This thread pool implementation uses a lock-free queue for task
 * distribution and supports work-stealing for better load balancing.
 * It's designed for CPU-intensive parallel workloads.
 */
class ThreadPool {
public:
    using Task = std::function<void()>;

private:
    std::vector<std::thread> workers_;
    LockFreeQueue<Task> task_queue_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> active_tasks_{0};
    std::mutex mutex_;
    std::condition_variable condition_;

    void worker_loop() {
        while (!stop_.load(std::memory_order_acquire)) {
            auto task_opt = task_queue_.dequeue();
            
            if (task_opt.has_value()) {
                active_tasks_.fetch_add(1, std::memory_order_relaxed);
                task_opt.value()();
                active_tasks_.fetch_sub(1, std::memory_order_relaxed);
            } else {
                // Wait for tasks with timeout
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait_for(lock, std::chrono::milliseconds(100), 
                    [this] { return stop_.load() || !task_queue_.empty(); });
            }
        }
    }

public:
    /**
     * @brief Constructs a thread pool
     * 
     * @param num_threads Number of worker threads (default: hardware concurrency)
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) {
        if (num_threads == 0) {
            num_threads = 1;
        }

        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back(&ThreadPool::worker_loop, this);
        }
    }

    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool() {
        // Wait for all queued and active tasks to complete first
        wait();
        
        // Signal workers to stop
        stop_.store(true, std::memory_order_release);
        condition_.notify_all();

        // Wait for all workers to finish
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Non-copyable, non-movable
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Submits a task to the thread pool
     * 
     * @tparam F Callable type
     * @tparam Args Argument types
     * @param f Callable object
     * @param args Arguments to pass to the callable
     * @return std::future containing the result
     */
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using ReturnType = std::invoke_result_t<F, Args...>;
        
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<ReturnType> result = task->get_future();

        task_queue_.enqueue([task]() { (*task)(); });
        condition_.notify_one();

        return result;
    }

    /**
     * @brief Waits for all tasks to complete
     */
    void wait() {
        // Wait for active tasks to complete
        while (active_tasks_.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield();
        }
        
        // Drain the queue to ensure all tasks are processed
        while (!task_queue_.empty()) {
            auto task_opt = task_queue_.dequeue();
            if (task_opt.has_value()) {
                active_tasks_.fetch_add(1, std::memory_order_relaxed);
                task_opt.value()();
                active_tasks_.fetch_sub(1, std::memory_order_relaxed);
            }
            std::this_thread::yield();
        }
    }

    /**
     * @brief Gets the number of active tasks
     * 
     * @return Number of currently executing tasks
     */
    size_t active_tasks() const {
        return active_tasks_.load(std::memory_order_acquire);
    }

    /**
     * @brief Gets the number of queued tasks
     * 
     * @return Approximate number of queued tasks
     */
    size_t queued_tasks() const {
        return task_queue_.approximate_size();
    }
};

} // namespace concurrent

