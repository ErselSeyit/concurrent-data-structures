cmake_minimum_required(VERSION 3.20)
project(ConcurrentDataStructures VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler-specific options
if(MSVC)
    add_compile_options(/W4 /permissive- /Zc:__cplusplus)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/lockfree_queue.cpp
    src/lockfree_hashmap.cpp
    src/thread_pool.cpp
)

# Header files
set(HEADERS
    include/concurrent/lockfree_queue.hpp
    include/concurrent/lockfree_hashmap.hpp
    include/concurrent/thread_pool.hpp
)

# Main library
add_library(concurrent_data_structures STATIC ${SOURCES} ${HEADERS})

target_compile_features(concurrent_data_structures PUBLIC cxx_std_20)

# Threading library
find_package(Threads REQUIRED)
target_link_libraries(concurrent_data_structures PUBLIC Threads::Threads)

# Benchmark executable
add_executable(benchmark benchmarks/main.cpp)
target_link_libraries(benchmark PRIVATE concurrent_data_structures)

# Test executable (using Google Test)
enable_testing()

# Download and configure Google Test
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# Test files
file(GLOB TEST_SOURCES "tests/*.cpp")
add_executable(tests ${TEST_SOURCES})
target_link_libraries(tests PRIVATE concurrent_data_structures gtest_main)
target_include_directories(tests PRIVATE ${googletest_SOURCE_DIR}/include)

# Example executable
add_executable(example examples/main.cpp)
target_link_libraries(example PRIVATE concurrent_data_structures)

