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
    # Use less strict warnings for GUI dependencies
    option(STRICT_WARNINGS "Use strict compiler warnings" ON)
    if(STRICT_WARNINGS)
        add_compile_options(-Wall -Wextra -Wpedantic -Werror)
    else()
        add_compile_options(-Wall -Wextra)
    endif()
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

# GUI executable (using ImGui and GLFW)
option(BUILD_GUI "Build GUI application" ON)

if(BUILD_GUI)
    # Temporarily disable strict warnings for GUI dependencies
    set(OLD_STRICT_WARNINGS ${STRICT_WARNINGS})
    set(STRICT_WARNINGS OFF)
    
    # Find OpenGL
    find_package(OpenGL REQUIRED)
    
    # Download and configure GLFW
    include(FetchContent)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)
    
    # Restore warning settings
    set(STRICT_WARNINGS ${OLD_STRICT_WARNINGS})
    
    # Download and configure ImGui
    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.89.9
    )
    FetchContent_MakeAvailable(imgui)
    
    # ImGui sources
    set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    
    # GUI executable
    add_executable(gui gui/main.cpp ${IMGUI_SOURCES})
    target_link_libraries(gui PRIVATE 
        concurrent_data_structures
        glfw
        OpenGL::GL
    )
    target_include_directories(gui PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    
    # Disable pedantic warnings for ImGui (it has some style issues)
    if(NOT MSVC)
        target_compile_options(gui PRIVATE -Wno-pedantic)
    endif()
endif()

