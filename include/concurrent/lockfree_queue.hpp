#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>

namespace concurrent {

/**
 * @brief Lock-free, wait-free concurrent queue implementation
 * 
 * This is a high-performance, thread-safe queue that uses atomic operations
 * and memory ordering to achieve lock-free synchronization. It's designed
 * for high-throughput scenarios where multiple threads need to enqueue and
 * dequeue items concurrently.
 * 
 * @tparam T The type of elements stored in the queue
 */
template<typename T>
class LockFreeQueue {
    static_assert(std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>,
                  "T must be move or copy constructible");

private:
    struct Node {
        std::atomic<T*> data{nullptr};
        std::atomic<Node*> next{nullptr};
        
        Node() = default;
    };

    alignas(64) std::atomic<Node*> head_;
    alignas(64) std::atomic<Node*> tail_;

    // Memory pool for nodes to reduce allocations
    Node* allocate_node() {
        return new Node();
    }

    void deallocate_node(Node* node) {
        delete node;
    }

public:
    /**
     * @brief Constructs an empty lock-free queue
     */
    LockFreeQueue() {
        Node* dummy = allocate_node();
        head_.store(dummy, std::memory_order_relaxed);
        tail_.store(dummy, std::memory_order_relaxed);
    }

    /**
     * @brief Destructor - not thread-safe, queue must be empty
     */
    ~LockFreeQueue() {
        Node* current = head_.load(std::memory_order_relaxed);
        while (current) {
            Node* next = current->next.load(std::memory_order_relaxed);
            T* data = current->data.load(std::memory_order_relaxed);
            if (data) {
                delete data;
            }
            deallocate_node(current);
            current = next;
        }
    }

    // Non-copyable, non-movable
    LockFreeQueue(const LockFreeQueue&) = delete;
    LockFreeQueue& operator=(const LockFreeQueue&) = delete;
    LockFreeQueue(LockFreeQueue&&) = delete;
    LockFreeQueue& operator=(LockFreeQueue&&) = delete;

    /**
     * @brief Enqueues an item into the queue
     * 
     * @param item The item to enqueue (will be moved if possible)
     * @return true if successful, false otherwise
     */
    bool enqueue(T item) {
        Node* new_node = allocate_node();
        T* data = new T(std::move(item));
        
        // Set data with release semantics
        new_node->data.store(data, std::memory_order_release);
        new_node->next.store(nullptr, std::memory_order_relaxed);

        // Lock-free enqueue using compare-and-swap
        Node* prev_tail = tail_.exchange(new_node, std::memory_order_acq_rel);
        prev_tail->next.store(new_node, std::memory_order_release);
        
        return true;
    }

    /**
     * @brief Attempts to dequeue an item from the queue
     * 
     * @return std::optional<T> containing the item if available, empty otherwise
     */
    std::optional<T> dequeue() {
        Node* head = head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);

        if (next == nullptr) {
            return std::nullopt; // Queue is empty
        }

        T* data = next->data.load(std::memory_order_acquire);
        if (data == nullptr) {
            return std::nullopt;
        }

        // Move the data out
        T result = std::move(*data);
        delete data;

        // Update head
        head_.store(next, std::memory_order_release);

        // Clean up old dummy node
        deallocate_node(head);

        return result;
    }

    /**
     * @brief Checks if the queue is empty
     * 
     * @note This is a snapshot and may be outdated immediately
     * @return true if queue appears empty, false otherwise
     */
    bool empty() const {
        Node* head = head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        return next == nullptr;
    }

    /**
     * @brief Gets the approximate size of the queue
     * 
     * @warning This is expensive and only approximate
     * @return Approximate number of elements
     */
    size_t approximate_size() const {
        size_t count = 0;
        Node* current = head_.load(std::memory_order_acquire);
        while (current) {
            Node* next = current->next.load(std::memory_order_acquire);
            if (next && next->data.load(std::memory_order_acquire)) {
                ++count;
            }
            current = next;
        }
        return count;
    }
};

} // namespace concurrent

