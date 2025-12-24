# High-Performance Concurrent Data Structures

A modern C++20 library implementing lock-free, high-performance concurrent data structures demonstrating advanced C++ programming skills and best practices.

## 🚀 Features

- **Lock-Free Queue**: Wait-free enqueue/dequeue operations using atomic operations
- **Lock-Free Hash Map**: High-performance concurrent hash map with fine-grained synchronization
- **Thread Pool**: Efficient thread pool with work-stealing capabilities
- **Modern C++20**: Utilizes latest C++ features (concepts, ranges, smart pointers, etc.)
- **Zero Dependencies**: Core library has no external dependencies (except for testing)
- **Comprehensive Tests**: Full test coverage using Google Test
- **Performance Benchmarks**: Built-in benchmarking suite

## 📋 Requirements

- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2019+)
- CMake 3.20 or higher
- (Optional) Google Test for running tests

## 🏗️ Building

```bash
# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run tests
ctest

# Run benchmarks
./benchmark

# Run examples
./example
```

## 📖 Usage Examples

### Lock-Free Queue

```cpp
#include "concurrent/lockfree_queue.hpp"

concurrent::LockFreeQueue<int> queue;

// Producer thread
queue.enqueue(42);

// Consumer thread
auto item = queue.dequeue();
if (item.has_value()) {
    std::cout << "Got: " << item.value() << std::endl;
}
```

### Lock-Free Hash Map

```cpp
#include "concurrent/lockfree_hashmap.hpp"

concurrent::LockFreeHashMap<std::string, int> map;

// Insert
map.insert("key", 100);

// Retrieve
if (auto value = map.get("key")) {
    std::cout << "Value: " << value.value() << std::endl;
}

// Check existence
if (map.contains("key")) {
    // ...
}

// Remove
map.erase("key");
```

### Thread Pool

```cpp
#include "concurrent/thread_pool.hpp"

concurrent::ThreadPool pool(4); // 4 worker threads

// Submit task
auto future = pool.submit([]() {
    return 42;
});

int result = future.get();

// Wait for all tasks
pool.wait();
```

## 🎯 Key Highlights

### 1. **Modern C++ Expertise**
- C++20 features (concepts, ranges, coroutines-ready design)
- RAII and smart pointers
- Move semantics and perfect forwarding
- Template metaprogramming

### 2. **Concurrency Mastery**
- Lock-free programming techniques
- Memory ordering and atomic operations
- Cache-line alignment for false sharing prevention
- Wait-free algorithms

### 3. **Performance Optimization**
- Zero-copy where possible
- Cache-friendly data structures
- Minimal memory allocations
- High-throughput design

### 4. **Software Engineering**
- Clean architecture and separation of concerns
- Comprehensive unit tests
- CMake build system
- Professional documentation

### 5. **Real-World Applicability**
- Production-ready code quality
- Thread-safe by design
- Scalable to many cores
- Suitable for high-frequency trading, game engines, and server applications

## 📊 Performance Characteristics

- **Queue**: O(1) enqueue/dequeue, lock-free, wait-free
- **Hash Map**: O(1) average case insert/lookup, lock-free reads
- **Thread Pool**: Minimal overhead, efficient work distribution

## 🧪 Testing

The project includes comprehensive unit tests covering:
- Basic functionality
- Concurrent operations
- Edge cases
- Thread safety
- Performance characteristics

Run tests with:
```bash
cd build
ctest --verbose
```

## 📝 Project Structure

```
.
├── CMakeLists.txt          # Build configuration
├── README.md              # This file
├── include/
│   └── concurrent/
│       ├── lockfree_queue.hpp
│       ├── lockfree_hashmap.hpp
│       └── thread_pool.hpp
├── src/
│   ├── lockfree_queue.cpp
│   ├── lockfree_hashmap.cpp
│   └── thread_pool.cpp
├── tests/
│   ├── test_lockfree_queue.cpp
│   ├── test_lockfree_hashmap.cpp
│   └── test_thread_pool.cpp
├── benchmarks/
│   └── main.cpp
└── examples/
    └── main.cpp
```

## 🔧 Technical Details

### Lock-Free Queue
- Uses atomic compare-and-swap operations
- Memory ordering: acquire-release semantics
- Node-based linked list structure
- Wait-free for both enqueue and dequeue

### Lock-Free Hash Map
- Bucket-based hash table
- Fine-grained synchronization per bucket
- Mark-and-sweep deletion strategy
- Configurable bucket count and hash function

### Thread Pool
- Lock-free task queue
- Work-stealing ready (can be extended)
- Efficient task distribution
- Graceful shutdown

## 🎓 Learning Outcomes

This project demonstrates:
- Advanced C++ programming
- Concurrent programming patterns
- Lock-free algorithm design
- Performance optimization techniques
- Professional software development practices

## 📄 License

This project is provided as a portfolio piece. Feel free to use it for learning and demonstration purposes.

## 🤝 Contributing

This is a showcase project, but suggestions and improvements are welcome!

---

**Built with passion for high-performance C++ programming** 🚀

