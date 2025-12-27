#pragma once

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace concurrent {

/**
 * @brief Lock-free concurrent hash map implementation
 * 
 * This is a high-performance, thread-safe hash map that uses fine-grained
 * locking with atomic operations for lock-free reads and lock-free writes
 * in most cases. Designed for high-concurrency scenarios.
 * 
 * @tparam Key The key type (must be hashable and equality comparable)
 * @tparam Value The value type
 * @tparam Hash The hash function type (defaults to std::hash<Key>)
 */
template<typename Key, typename Value, typename Hash = std::hash<Key>>
class LockFreeHashMap {
private:
    struct Node {
        Key key;
        std::atomic<Value*> value;
        std::atomic<Node*> next{nullptr};
        std::atomic<bool> marked{false}; // For safe deletion

        Node(const Key& k, const Value& v) 
            : key(k), value(new Value(v)) {}
        
        ~Node() {
            Value* val = value.load();
            if (val) {
                delete val;
            }
        }
    };

    struct Bucket {
        alignas(64) std::atomic<Node*> head{nullptr};
    };

    static constexpr size_t DEFAULT_BUCKET_COUNT = 1024;
    static constexpr double LOAD_FACTOR_THRESHOLD = 0.75;

    std::vector<Bucket> buckets_;
    std::atomic<size_t> size_{0};
    Hash hasher_;

    size_t bucket_index(const Key& key) const {
        return hasher_(key) % buckets_.size();
    }

    Node* find_node(const Bucket& bucket, const Key& key) const {
        Node* current = bucket.head.load(std::memory_order_acquire);
        while (current) {
            if (!current->marked.load(std::memory_order_acquire) && 
                current->key == key) {
                return current;
            }
            current = current->next.load(std::memory_order_acquire);
        }
        return nullptr;
    }

public:
    /**
     * @brief Constructs a lock-free hash map
     * 
     * @param bucket_count Initial number of buckets (default: 1024)
     * @param hash Hash function instance
     */
    explicit LockFreeHashMap(size_t bucket_count = DEFAULT_BUCKET_COUNT, 
                            Hash hash = Hash())
        : buckets_(bucket_count), hasher_(std::move(hash)) {}

    /**
     * @brief Inserts or updates a key-value pair
     * 
     * @param key The key
     * @param value The value
     * @return true if inserted, false if updated
     */
    bool insert(const Key& key, const Value& value) {
        Bucket& bucket = buckets_[bucket_index(key)];
        
        // Check if key already exists
        Node* existing = find_node(bucket, key);
        if (existing) {
            // Update existing value
            Value* new_val = new Value(value);
            Value* old_val = existing->value.exchange(new_val, std::memory_order_acq_rel);
            delete old_val;
            return false;
        }

        // Insert new node
        Node* new_node = new Node(key, value);
        Node* head = bucket.head.load(std::memory_order_acquire);
        new_node->next.store(head, std::memory_order_relaxed);

        // Try to update head atomically
        while (!bucket.head.compare_exchange_weak(
            head, new_node, 
            std::memory_order_release, 
            std::memory_order_acquire)) {
            new_node->next.store(head, std::memory_order_relaxed);
        }

        size_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    /**
     * @brief Retrieves a value by key
     * 
     * @param key The key to look up
     * @return std::optional<Value> containing the value if found
     */
    std::optional<Value> get(const Key& key) const {
        const Bucket& bucket = buckets_[bucket_index(key)];
        Node* node = find_node(bucket, key);
        
        if (node) {
            Value* val = node->value.load(std::memory_order_acquire);
            if (val) {
                return *val;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Removes a key-value pair
     * 
     * @param key The key to remove
     * @return true if removed, false if not found
     */
    bool erase(const Key& key) {
        Bucket& bucket = buckets_[bucket_index(key)];
        
        // Retry loop for finding and removing the node
        while (true) {
            Node* node = find_node(bucket, key);
            
            if (!node) {
                return false;
            }

            // Mark node for deletion (prevents other threads from finding it)
            bool was_marked = node->marked.exchange(true, std::memory_order_acq_rel);
            if (was_marked) {
                // Node was already marked by another thread, retry to find it again
                // (it might have been removed already)
                continue;
            }
            
            // Successfully marked the node - we own it now
            // Remove from chain
            Node* head = bucket.head.load(std::memory_order_acquire);
            if (head == node) {
                // Node is at head - try to update head
                Node* next = node->next.load(std::memory_order_acquire);
                while (!bucket.head.compare_exchange_weak(
                    head, next,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                    if (head != node) {
                        // Head changed to a different node - restart from beginning
                        // (another thread might have removed our node or changed the chain)
                        continue;
                    }
                    next = node->next.load(std::memory_order_acquire);
                }
            } else {
                // Find previous node and remove from middle
                Node* prev = head;
                bool removed = false;
                while (prev && !removed) {
                    Node* next = prev->next.load(std::memory_order_acquire);
                    if (next == node) {
                        Node* node_next = node->next.load(std::memory_order_acquire);
                        if (prev->next.compare_exchange_weak(
                            next, node_next,
                            std::memory_order_release,
                            std::memory_order_acquire)) {
                            removed = true;
                        } else {
                            // CAS failed, chain changed - restart search from beginning
                            break;
                        }
                    } else if (next == nullptr) {
                        // Reached end of chain, node not found (might have been removed)
                        break;
                    }
                    prev = next;
                }
                
                // If removal failed, retry from the beginning
                if (!removed) {
                    continue;
                }
            }

            // Successfully removed from chain, now safe to delete
            size_.fetch_sub(1, std::memory_order_relaxed);
            
            // Delete value first
            Value* val = node->value.exchange(nullptr, std::memory_order_acq_rel);
            if (val) {
                delete val;
            }
            
            // Delete node (safe now as it's removed from chain and marked)
            delete node;
            return true;
        }
    }

    /**
     * @brief Checks if a key exists
     * 
     * @param key The key to check
     * @return true if key exists, false otherwise
     */
    bool contains(const Key& key) const {
        const Bucket& bucket = buckets_[bucket_index(key)];
        return find_node(bucket, key) != nullptr;
    }

    /**
     * @brief Gets the approximate size
     * 
     * @return Approximate number of elements
     */
    size_t size() const noexcept {
        return size_.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if the map is empty
     * 
     * @return true if empty, false otherwise
     */
    bool empty() const noexcept {
        return size() == 0;
    }
};

} // namespace concurrent

