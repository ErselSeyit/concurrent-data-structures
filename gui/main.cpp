#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "concurrent/lockfree_queue.hpp"
#include "concurrent/lockfree_hashmap.hpp"
#include "concurrent/thread_pool.hpp"
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <mutex>
#include <atomic>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <map>
#include <deque>
#include <algorithm>

using namespace concurrent;

// Global data structures
LockFreeQueue<int> g_queue;
LockFreeHashMap<std::string, int> g_hashmap;
std::unique_ptr<ThreadPool> g_thread_pool;

// Enhanced statistics
struct Stats {
    std::atomic<size_t> queue_enqueued{0};
    std::atomic<size_t> queue_dequeued{0};
    std::atomic<size_t> hashmap_inserts{0};
    std::atomic<size_t> hashmap_gets{0};
    std::atomic<size_t> hashmap_erases{0};
    std::atomic<size_t> thread_pool_tasks_submitted{0};
    std::atomic<size_t> thread_pool_tasks_completed{0};
    
    // History data
    std::vector<float> queue_size_history;
    std::vector<float> active_tasks_history;
    std::vector<float> throughput_history;
    std::vector<float> latency_history;
    std::mutex history_mutex;
    
    // Latency tracking
    std::deque<std::chrono::microseconds> operation_times;
    std::mutex latency_mutex;
    const size_t max_latency_samples = 1000;
    
    // Hash map snapshot for display
    std::map<std::string, int> hashmap_snapshot;
    std::mutex snapshot_mutex;
    
    // Queue contents snapshot
    std::vector<int> queue_snapshot;
    std::mutex queue_snapshot_mutex;
    
    void add_queue_size(float size) {
        std::lock_guard<std::mutex> lock(history_mutex);
        queue_size_history.push_back(size);
        if (queue_size_history.size() > 500) {
            queue_size_history.erase(queue_size_history.begin());
        }
    }
    
    void add_active_tasks(float tasks) {
        std::lock_guard<std::mutex> lock(history_mutex);
        active_tasks_history.push_back(tasks);
        if (active_tasks_history.size() > 500) {
            active_tasks_history.erase(active_tasks_history.begin());
        }
    }
    
    void add_throughput(float throughput) {
        std::lock_guard<std::mutex> lock(history_mutex);
        throughput_history.push_back(throughput);
        if (throughput_history.size() > 500) {
            throughput_history.erase(throughput_history.begin());
        }
    }
    
    void record_operation_time(std::chrono::microseconds time) {
        std::lock_guard<std::mutex> lock(latency_mutex);
        operation_times.push_back(time);
        if (operation_times.size() > max_latency_samples) {
            operation_times.pop_front();
        }
        
        // Update latency history
        std::lock_guard<std::mutex> hist_lock(history_mutex);
        latency_history.push_back(static_cast<float>(time.count()));
        if (latency_history.size() > 500) {
            latency_history.erase(latency_history.begin());
        }
    }
    
    float get_avg_latency() {
        std::lock_guard<std::mutex> lock(latency_mutex);
        if (operation_times.empty()) return 0.0f;
        float sum = 0.0f;
        for (const auto& t : operation_times) {
            sum += static_cast<float>(t.count());
        }
        return sum / operation_times.size();
    }
    
    float get_max_latency() {
        std::lock_guard<std::mutex> lock(latency_mutex);
        if (operation_times.empty()) return 0.0f;
        auto max_it = std::max_element(operation_times.begin(), operation_times.end());
        return static_cast<float>(max_it->count());
    }
    
    float get_min_latency() {
        std::lock_guard<std::mutex> lock(latency_mutex);
        if (operation_times.empty()) return 0.0f;
        auto min_it = std::min_element(operation_times.begin(), operation_times.end());
        return static_cast<float>(min_it->count());
    }
} g_stats;

// Auto-producer/consumer threads
std::atomic<bool> g_auto_producer_running{false};
std::atomic<bool> g_auto_consumer_running{false};
std::thread g_producer_thread;
std::thread g_consumer_thread;

void auto_producer() {
    int counter = 0;
    while (g_auto_producer_running.load()) {
        auto start = std::chrono::high_resolution_clock::now();
        g_queue.enqueue(counter++);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        g_stats.record_operation_time(duration);
        g_stats.queue_enqueued.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void auto_consumer() {
    while (g_auto_consumer_running.load()) {
        auto start = std::chrono::high_resolution_clock::now();
        auto item = g_queue.dequeue();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        if (item.has_value()) {
            g_stats.record_operation_time(duration);
            g_stats.queue_dequeued.fetch_add(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

// Export statistics to file
void export_stats(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    file << "Concurrent Data Structures Statistics Export\n";
    file << "==========================================\n\n";
    
    file << "Queue Statistics:\n";
    file << "  Enqueued: " << g_stats.queue_enqueued.load() << "\n";
    file << "  Dequeued: " << g_stats.queue_dequeued.load() << "\n";
    file << "  Current Size: " << g_queue.approximate_size() << "\n\n";
    
    file << "Hash Map Statistics:\n";
    file << "  Inserts: " << g_stats.hashmap_inserts.load() << "\n";
    file << "  Gets: " << g_stats.hashmap_gets.load() << "\n";
    file << "  Erases: " << g_stats.hashmap_erases.load() << "\n";
    file << "  Current Size: " << g_hashmap.size() << "\n\n";
    
    file << "Thread Pool Statistics:\n";
    file << "  Tasks Submitted: " << g_stats.thread_pool_tasks_submitted.load() << "\n";
    file << "  Tasks Completed: " << g_stats.thread_pool_tasks_completed.load() << "\n\n";
    
    file << "Performance Metrics:\n";
    file << "  Average Latency: " << g_stats.get_avg_latency() << " microseconds\n";
    file << "  Min Latency: " << g_stats.get_min_latency() << " microseconds\n";
    file << "  Max Latency: " << g_stats.get_max_latency() << " microseconds\n";
    
    file.close();
}

// Custom color scheme
void setup_custom_style() {
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Colors
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.00f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.20f, 0.40f, 0.80f, 0.50f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.45f, 0.85f, 0.80f);
    style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.80f, 0.60f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.45f, 0.85f, 0.80f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.50f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.30f, 1.00f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.45f, 0.85f, 0.80f);
    style.Colors[ImGuiCol_TabActive] = ImVec4(0.20f, 0.40f, 0.80f, 1.00f);
    style.Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.70f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    
    // Rounding
    style.WindowRounding = 5.0f;
    style.FrameRounding = 3.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 3.0f;
    
    // Spacing
    style.ItemSpacing = ImVec2(8, 6);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.FramePadding = ImVec2(6, 4);
}

int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        return -1;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(1800, 1200, "Concurrent Data Structures Monitor", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // Get DPI scaling
    float xscale, yscale;
    glfwGetWindowContentScale(window, &xscale, &yscale);
    
    int width = static_cast<int>(1800 * xscale);
    int height = static_cast<int>(1200 * yscale);
    glfwSetWindowSize(window, width, height);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    
    if (xscale > 1.0f || yscale > 1.0f) {
        ImGui::GetStyle().ScaleAllSizes(std::max(xscale, yscale));
        io.FontGlobalScale = std::max(xscale, yscale);
    }

    setup_custom_style();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    g_thread_pool = std::make_unique<ThreadPool>(4);

    auto last_update = std::chrono::steady_clock::now();
    auto last_throughput_calc = std::chrono::steady_clock::now();
    size_t last_total_ops = 0;
    
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Update statistics
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update).count() > 100) {
            g_stats.add_queue_size(static_cast<float>(g_queue.approximate_size()));
            if (g_thread_pool) {
                g_stats.add_active_tasks(static_cast<float>(g_thread_pool->active_tasks()));
            }
            last_update = now;
        }
        
        // Calculate throughput
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_throughput_calc).count() > 1000) {
            size_t current_total = g_stats.queue_enqueued.load() + g_stats.queue_dequeued.load();
            float throughput = static_cast<float>(current_total - last_total_ops);
            g_stats.add_throughput(throughput);
            last_total_ops = current_total;
            last_throughput_calc = now;
        }

        // Main window
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        
        ImGui::Begin("Concurrent Data Structures Monitor", nullptr, 
                     ImGuiWindowFlags_MenuBar | 
                     ImGuiWindowFlags_NoTitleBar | 
                     ImGuiWindowFlags_NoCollapse | 
                     ImGuiWindowFlags_NoResize | 
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("Clear Queue")) {
                while (!g_queue.empty()) {
                    g_queue.dequeue();
                }
            }
            if (ImGui::MenuItem("Export Stats")) {
                export_stats("stats_export.txt");
            }
            if (ImGui::MenuItem("Reset Stats")) {
                g_stats.queue_enqueued.store(0);
                g_stats.queue_dequeued.store(0);
                g_stats.hashmap_inserts.store(0);
                g_stats.hashmap_gets.store(0);
                g_stats.hashmap_erases.store(0);
                g_stats.thread_pool_tasks_submitted.store(0);
                g_stats.thread_pool_tasks_completed.store(0);
            }
            ImGui::EndMenuBar();
        }

        // Tabs
        if (ImGui::BeginTabBar("MainTabs")) {
            // Queue Tab
            if (ImGui::BeginTabItem("Queue")) {
                ImGui::Spacing();
                
                // Statistics cards
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("QueueStats", ImVec2(300, 120), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Queue Statistics");
                ImGui::Separator();
                ImGui::Text("Size: %zu", g_queue.approximate_size());
                ImGui::Text("Enqueued: %zu", g_stats.queue_enqueued.load());
                ImGui::Text("Dequeued: %zu", g_stats.queue_dequeued.load());
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::SameLine();
                
                // Controls
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("QueueControls", ImVec2(400, 120), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Controls");
                ImGui::Separator();
                
                static int queue_value = 0;
                ImGui::InputInt("Value", &queue_value, 1, 10);
                ImGui::SameLine();
                if (ImGui::Button("Enqueue", ImVec2(80, 0))) {
                    auto start = std::chrono::high_resolution_clock::now();
                    g_queue.enqueue(queue_value);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    g_stats.record_operation_time(duration);
                    g_stats.queue_enqueued.fetch_add(1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Dequeue", ImVec2(80, 0))) {
                    auto start = std::chrono::high_resolution_clock::now();
                    auto item = g_queue.dequeue();
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    if (item.has_value()) {
                        g_stats.record_operation_time(duration);
                        queue_value = item.value();
                        g_stats.queue_dequeued.fetch_add(1);
                    }
                }
                
                ImGui::Spacing();
                bool auto_prod = g_auto_producer_running.load();
                if (ImGui::Checkbox("Auto Producer", &auto_prod)) {
                    g_auto_producer_running.store(auto_prod);
                    if (auto_prod && !g_producer_thread.joinable()) {
                        g_producer_thread = std::thread(auto_producer);
                    }
                }
                ImGui::SameLine();
                bool auto_cons = g_auto_consumer_running.load();
                if (ImGui::Checkbox("Auto Consumer", &auto_cons)) {
                    g_auto_consumer_running.store(auto_cons);
                    if (auto_cons && !g_consumer_thread.joinable()) {
                        g_consumer_thread = std::thread(auto_consumer);
                    }
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::Spacing();
                
                // Queue visualization
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("QueueViz", ImVec2(-1, 150), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Queue Contents (Last 50 items)");
                ImGui::Separator();
                
                // Sample queue contents for visualization
                std::vector<int> sample;
                LockFreeQueue<int> temp_queue;
                int count = 0;
                while (!g_queue.empty() && count < 50) {
                    auto item = g_queue.dequeue();
                    if (item.has_value()) {
                        sample.push_back(item.value());
                        temp_queue.enqueue(item.value());
                    }
                    count++;
                }
                while (!temp_queue.empty()) {
                    auto item = temp_queue.dequeue();
                    if (item.has_value()) {
                        g_queue.enqueue(item.value());
                    }
                }
                
                if (!sample.empty()) {
                    ImGui::Text("Front -> ");
                    for (size_t i = 0; i < sample.size() && i < 50; ++i) {
                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.6f));
                        ImGui::Button(std::to_string(sample[i]).c_str(), ImVec2(40, 30));
                        ImGui::PopStyleColor();
                    }
                    ImGui::Text(" <- Back");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Queue is empty");
                }
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::Spacing();
                
                // Graphs
                {
                    std::lock_guard<std::mutex> lock(g_stats.history_mutex);
                    if (!g_stats.queue_size_history.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("QueueGraph", ImVec2(-1, 200), true);
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Queue Size History");
                        float max_val = *std::max_element(g_stats.queue_size_history.begin(), g_stats.queue_size_history.end());
                        ImGui::PlotLines("", g_stats.queue_size_history.data(), 
                                        static_cast<int>(g_stats.queue_size_history.size()), 
                                        0, nullptr, 0.0f, std::max(max_val * 1.2f, 10.0f), ImVec2(-1, 150));
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // Hash Map Tab
            if (ImGui::BeginTabItem("Hash Map")) {
                ImGui::Spacing();
                
                // Statistics
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("HashMapStats", ImVec2(300, 120), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Hash Map Statistics");
                ImGui::Separator();
                ImGui::Text("Size: %zu", g_hashmap.size());
                ImGui::Text("Inserts: %zu", g_stats.hashmap_inserts.load());
                ImGui::Text("Gets: %zu", g_stats.hashmap_gets.load());
                ImGui::Text("Erases: %zu", g_stats.hashmap_erases.load());
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::SameLine();
                
                // Controls
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("HashMapControls", ImVec2(500, 120), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Controls");
                ImGui::Separator();
                
                static char key_buffer[256] = "key";
                static int map_value = 0;
                ImGui::InputText("Key", key_buffer, sizeof(key_buffer));
                ImGui::SameLine();
                ImGui::InputInt("Value", &map_value);
                
                if (ImGui::Button("Insert/Update", ImVec2(100, 0))) {
                    auto start = std::chrono::high_resolution_clock::now();
                    g_hashmap.insert(std::string(key_buffer), map_value);
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    g_stats.record_operation_time(duration);
                    g_stats.hashmap_inserts.fetch_add(1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Get", ImVec2(80, 0))) {
                    auto start = std::chrono::high_resolution_clock::now();
                    auto val = g_hashmap.get(std::string(key_buffer));
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    g_stats.record_operation_time(duration);
                    if (val.has_value()) {
                        map_value = val.value();
                        g_stats.hashmap_gets.fetch_add(1);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Erase", ImVec2(80, 0))) {
                    auto start = std::chrono::high_resolution_clock::now();
                    g_hashmap.erase(std::string(key_buffer));
                    auto end = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
                    g_stats.record_operation_time(duration);
                    g_stats.hashmap_erases.fetch_add(1);
                }
                ImGui::SameLine();
                if (ImGui::Button("Contains", ImVec2(80, 0))) {
                    bool contains = g_hashmap.contains(std::string(key_buffer));
                    if (contains) {
                        ImGui::OpenPopup("KeyExists");
                    } else {
                        ImGui::OpenPopup("KeyNotFound");
                    }
                }
                
                if (ImGui::BeginPopup("KeyExists")) {
                    ImGui::Text("Key exists!");
                    ImGui::EndPopup();
                }
                if (ImGui::BeginPopup("KeyNotFound")) {
                    ImGui::Text("Key not found");
                    ImGui::EndPopup();
                }
                
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::Spacing();
                
                // Hash Map Contents Table
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("HashMapContents", ImVec2(-1, 400), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Hash Map Contents");
                ImGui::Separator();
                
                // Create snapshot of hash map (this is approximate for display)
                // Note: This is a simplified approach - in production you'd want a better way
                static std::vector<std::pair<std::string, int>> display_items;
                display_items.clear();
                
                // We can't easily iterate the hash map, so we'll show a message
                // In a real implementation, you'd add iteration support to the hash map
                if (g_hashmap.size() > 0) {
                    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), 
                        "Hash map contains %zu entries. Use Get operation to retrieve values.", g_hashmap.size());
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), 
                        "Note: Full iteration not implemented - use Get with known keys.");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Hash map is empty");
                }
                
                ImGui::EndChild();
                ImGui::PopStyleColor();
                
                ImGui::EndTabItem();
            }
            
            // Thread Pool Tab
            if (ImGui::BeginTabItem("Thread Pool")) {
                ImGui::Spacing();
                
                // Statistics
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("ThreadPoolStats", ImVec2(300, 150), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Thread Pool Statistics");
                ImGui::Separator();
                if (g_thread_pool) {
                    ImGui::Text("Active Tasks: %zu", g_thread_pool->active_tasks());
                    ImGui::Text("Queued Tasks: %zu", g_thread_pool->queued_tasks());
                }
                ImGui::Text("Submitted: %zu", g_stats.thread_pool_tasks_submitted.load());
                ImGui::Text("Completed: %zu", g_stats.thread_pool_tasks_completed.load());
                ImGui::Text("Workers: 4");
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::SameLine();
                
                // Controls
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("ThreadPoolControls", ImVec2(400, 150), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Controls");
                ImGui::Separator();
                
                if (ImGui::Button("Submit Test Task", ImVec2(150, 0))) {
                    if (g_thread_pool) {
                        g_thread_pool->submit([]() {
                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                            g_stats.thread_pool_tasks_completed.fetch_add(1);
                            return 42;
                        });
                        g_stats.thread_pool_tasks_submitted.fetch_add(1);
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Submit 10 Tasks", ImVec2(150, 0))) {
                    if (g_thread_pool) {
                        for (int i = 0; i < 10; ++i) {
                            g_thread_pool->submit([i]() {
                                std::this_thread::sleep_for(std::chrono::milliseconds(100 + i * 10));
                                g_stats.thread_pool_tasks_completed.fetch_add(1);
                                return i;
                            });
                            g_stats.thread_pool_tasks_submitted.fetch_add(1);
                        }
                    }
                }
                
                ImGui::Spacing();
                if (ImGui::Button("Submit 100 Tasks", ImVec2(150, 0))) {
                    if (g_thread_pool) {
                        for (int i = 0; i < 100; ++i) {
                            g_thread_pool->submit([i]() {
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                g_stats.thread_pool_tasks_completed.fetch_add(1);
                                return i;
                            });
                            g_stats.thread_pool_tasks_submitted.fetch_add(1);
                        }
                    }
                }
                
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::Spacing();
                
                // Graphs
                {
                    std::lock_guard<std::mutex> lock(g_stats.history_mutex);
                    if (!g_stats.active_tasks_history.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("ThreadPoolGraph", ImVec2(-1, 200), true);
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Active Tasks History");
                        float max_val = *std::max_element(g_stats.active_tasks_history.begin(), g_stats.active_tasks_history.end());
                        ImGui::PlotLines("", g_stats.active_tasks_history.data(), 
                                        static_cast<int>(g_stats.active_tasks_history.size()), 
                                        0, nullptr, 0.0f, std::max(max_val * 1.2f, 5.0f), ImVec2(-1, 150));
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            // Performance Tab
            if (ImGui::BeginTabItem("Performance")) {
                ImGui::Spacing();
                
                // Performance metrics
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("PerfMetrics", ImVec2(400, 200), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Performance Metrics");
                ImGui::Separator();
                
                float throughput = 0.0f;
                {
                    std::lock_guard<std::mutex> lock(g_stats.history_mutex);
                    if (!g_stats.throughput_history.empty()) {
                        throughput = g_stats.throughput_history.back();
                    }
                }
                ImGui::Text("Queue Throughput: %.1f ops/sec", throughput);
                ImGui::Text("Avg Latency: %.2f μs", g_stats.get_avg_latency());
                ImGui::Text("Min Latency: %.2f μs", g_stats.get_min_latency());
                ImGui::Text("Max Latency: %.2f μs", g_stats.get_max_latency());
                
                size_t total_ops = g_stats.queue_enqueued.load() + g_stats.queue_dequeued.load() +
                                  g_stats.hashmap_inserts.load() + g_stats.hashmap_gets.load() +
                                  g_stats.hashmap_erases.load();
                ImGui::Text("Total Operations: %zu", total_ops);
                
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::SameLine();
                
                // Latency distribution
                ImGui::BeginGroup();
                ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                ImGui::BeginChild("LatencyDist", ImVec2(400, 200), true);
                ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Latency Distribution");
                ImGui::Separator();
                
                float avg = g_stats.get_avg_latency();
                float min = g_stats.get_min_latency();
                float max = g_stats.get_max_latency();
                
                if (max > 0) {
                    ImGui::Text("Min: %.2f μs", min);
                    ImGui::ProgressBar(min / max, ImVec2(-1, 20), "Min");
                    
                    ImGui::Text("Avg: %.2f μs", avg);
                    ImGui::ProgressBar(avg / max, ImVec2(-1, 20), "Avg");
                    
                    ImGui::Text("Max: %.2f μs", max);
                    ImGui::ProgressBar(1.0f, ImVec2(-1, 20), "Max");
                } else {
                    ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No latency data yet");
                }
                
                ImGui::EndChild();
                ImGui::PopStyleColor();
                ImGui::EndGroup();
                
                ImGui::Spacing();
                
                // Throughput graph
                {
                    std::lock_guard<std::mutex> lock(g_stats.history_mutex);
                    if (!g_stats.throughput_history.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("ThroughputGraph", ImVec2(-1, 200), true);
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Throughput History (ops/sec)");
                        float max_val = *std::max_element(g_stats.throughput_history.begin(), g_stats.throughput_history.end());
                        ImGui::PlotLines("", g_stats.throughput_history.data(), 
                                        static_cast<int>(g_stats.throughput_history.size()), 
                                        0, nullptr, 0.0f, std::max(max_val * 1.2f, 10.0f), ImVec2(-1, 150));
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }
                }
                
                // Latency graph
                {
                    std::lock_guard<std::mutex> lock(g_stats.history_mutex);
                    if (!g_stats.latency_history.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.20f, 1.0f));
                        ImGui::BeginChild("LatencyGraph", ImVec2(-1, 200), true);
                        ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Latency History (microseconds)");
                        float max_val = *std::max_element(g_stats.latency_history.begin(), g_stats.latency_history.end());
                        ImGui::PlotLines("", g_stats.latency_history.data(), 
                                        static_cast<int>(g_stats.latency_history.size()), 
                                        0, nullptr, 0.0f, std::max(max_val * 1.2f, 10.0f), ImVec2(-1, 150));
                        ImGui::EndChild();
                        ImGui::PopStyleColor();
                    }
                }
                
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::End();

        // Render
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    g_auto_producer_running.store(false);
    g_auto_consumer_running.store(false);
    if (g_producer_thread.joinable()) {
        g_producer_thread.join();
    }
    if (g_consumer_thread.joinable()) {
        g_consumer_thread.join();
    }
    
    g_thread_pool.reset();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
