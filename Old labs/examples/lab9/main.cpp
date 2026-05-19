// САМЫЕ ПЕРВЫЕ СТРОКИ!
#define NOMINMAX
#include <windows.h>

// Теперь остальные инклюды
#include <zmq.hpp>
#include <nlohmann/json.hpp>

#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <queue>

// ImGui headers
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

using json = nlohmann::json;

// Структура для хранения данных о местоположении
struct LocationData {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    float accuracy = 0.0f;
    std::string provider;
    std::string timestamp;
    long long unix_timestamp = 0;
    
    std::string toString() const {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(6)
           << "Lat: " << latitude << ", Lon: " << longitude
           << ", Alt: " << altitude << "m, Acc: " << accuracy << "m"
           << "\nProvider: " << provider
           << "\nTime: " << timestamp;
        return ss.str();
    }
};

// Общая структура данных для потоков
class SharedData {
private:
    std::mutex mtx;
    std::vector<LocationData> location_history;
    LocationData latest_location;
    std::atomic<int> message_count{0};
    std::atomic<bool> new_data_available{false};
    
public:
    void addLocation(const LocationData& loc) {
        std::lock_guard<std::mutex> lock(mtx);
        latest_location = loc;
        location_history.push_back(loc);
        message_count++;
        new_data_available = true;
        
        if (location_history.size() > 100) {
            location_history.erase(location_history.begin());
        }
    }
    
    LocationData getLatestLocation() {
        std::lock_guard<std::mutex> lock(mtx);
        new_data_available = false;
        return latest_location;
    }
    
    std::vector<LocationData> getHistory() {
        std::lock_guard<std::mutex> lock(mtx);
        return location_history;
    }
    
    bool hasNewData() const {
        return new_data_available;
    }
    
    int getMessageCount() const {
        return message_count;
    }
};

SharedData g_shared_data;
std::atomic<bool> g_server_running{true};

void saveLocationToJson(const LocationData& loc, const std::string& filename = "locations.json") {
    json j;
    
    std::ifstream in_file(filename);
    if (in_file.is_open()) {
        try {
            in_file >> j;
        } catch (...) {
            j = json::object();
        }
        in_file.close();
    }
    
    if (!j.contains("locations")) {
        j["locations"] = json::array();
    }
    
    json location_json = {
        {"latitude", loc.latitude},
        {"longitude", loc.longitude},
        {"altitude", loc.altitude},
        {"accuracy", loc.accuracy},
        {"provider", loc.provider},
        {"timestamp", loc.timestamp},
        {"unix_timestamp", loc.unix_timestamp}
    };
    
    j["locations"].push_back(location_json);
    j["last_update"] = loc.timestamp;
    j["total_count"] = j["locations"].size();
    
    std::ofstream out_file(filename);
    out_file << j.dump(2);
    out_file.close();
    
    std::cout << "[JSON] Saved location to " << filename << std::endl;
}

void zmq_server_thread() {
    zmq::context_t context(1);
    zmq::socket_t socket(context, zmq::socket_type::rep);
    
    const int port = 5556;
    
    try {
        socket.bind("tcp://*:" + std::to_string(port));
        std::cout << "[ZMQ] Server started on port " << port << std::endl;
        
        socket.set(zmq::sockopt::rcvtimeo, 1000);
        
        while (g_server_running) {
            zmq::message_t request;
            
            auto result = socket.recv(request, zmq::recv_flags::none);
            
            if (!result) {
                continue;
            }
            
            try {
                std::string message_str(static_cast<char*>(request.data()), request.size());
                std::cout << "[ZMQ] Received: " << message_str << std::endl;
                
                json received_json = json::parse(message_str);
                
                LocationData loc;
                
                if (received_json.contains("latitude")) {
                    loc.latitude = received_json["latitude"];
                    loc.longitude = received_json["longitude"];
                    loc.altitude = received_json.contains("altitude") && !received_json["altitude"].is_null() 
                                 ? received_json["altitude"].get<double>() : 0.0;
                    loc.accuracy = received_json.contains("accuracy") ? received_json["accuracy"].get<float>() : 0.0f;
                    loc.provider = received_json.contains("provider") ? received_json["provider"].get<std::string>() : "unknown";
                    loc.timestamp = received_json.contains("timestamp") ? received_json["timestamp"].get<std::string>() : "";
                    loc.unix_timestamp = received_json.contains("unix_timestamp") 
                                       ? received_json["unix_timestamp"].get<long long>() : 0;
                } else {
                    auto now = std::chrono::system_clock::now();
                    auto time_t_now = std::chrono::system_clock::to_time_t(now);
                    std::stringstream ss;
                    ss << std::put_time(std::localtime(&time_t_now), "%d.%m.%Y %H:%M:%S");
                    
                    loc.latitude = 0.0;
                    loc.longitude = 0.0;
                    loc.altitude = 0.0;
                    loc.accuracy = 0.0f;
                    loc.provider = "test";
                    loc.timestamp = ss.str();
                    loc.unix_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                        now.time_since_epoch()).count();
                }
                
                g_shared_data.addLocation(loc);
                saveLocationToJson(loc);
                
                std::string reply = "Location received. Total: " + std::to_string(g_shared_data.getMessageCount());
                socket.send(zmq::buffer(reply), zmq::send_flags::none);
                
            } catch (const json::parse_error& e) {
                std::cerr << "[ZMQ] JSON parse error: " << e.what() << std::endl;
                std::string reply = "ERROR: Invalid JSON format";
                socket.send(zmq::buffer(reply), zmq::send_flags::none);
            } catch (const std::exception& e) {
                std::cerr << "[ZMQ] Error: " << e.what() << std::endl;
                std::string reply = "ERROR: Processing failed";
                socket.send(zmq::buffer(reply), zmq::send_flags::none);
            }
        }
        
    } catch (const zmq::error_t& e) {
        std::cerr << "[ZMQ] Error: " << e.what() << std::endl;
    }
    
    socket.close();
    context.close();
    std::cout << "[ZMQ] Server stopped" << std::endl;
}

void gui_thread() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }
    
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Location Tracker Server", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    
    LocationData current_display_location;
    bool auto_update = true;
    std::vector<LocationData> history;
    
    std::cout << "[GUI] Window initialized" << std::endl;
    
    while (!glfwWindowShouldClose(window) && g_server_running) {
        glfwPollEvents();
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        if (g_shared_data.hasNewData() && auto_update) {
            current_display_location = g_shared_data.getLatestLocation();
        }
        
        ImGui::SetNextWindowSize(ImVec2(780, 580), ImGuiCond_FirstUseEver);
        ImGui::Begin("Location Information");
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
                          "Total Messages Received: %d", g_shared_data.getMessageCount());
        
        ImGui::Separator();
        
        ImGui::Text("Latest Location Data:");
        ImGui::Spacing();
        
        if (current_display_location.unix_timestamp > 0) {
            ImGui::Text("Latitude:  %.6f", current_display_location.latitude);
            ImGui::Text("Longitude: %.6f", current_display_location.longitude);
            ImGui::Text("Altitude:  %.2f m", current_display_location.altitude);
            ImGui::Text("Accuracy:  %.2f m", current_display_location.accuracy);
            ImGui::Text("Provider:  %s", current_display_location.provider.c_str());
            ImGui::Text("Time:      %s", current_display_location.timestamp.c_str());
            ImGui::Text("Unix Time: %lld", current_display_location.unix_timestamp);
        } else {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), 
                              "Waiting for location data...");
        }
        
        ImGui::Separator();
        
        ImGui::Checkbox("Auto-update", &auto_update);
        
        if (ImGui::Button("Manual Update")) {
            current_display_location = g_shared_data.getLatestLocation();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Show History")) {
            history = g_shared_data.getHistory();
            ImGui::OpenPopup("Location History");
        }
        
        if (ImGui::BeginPopupModal("Location History", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Last %zu locations:", history.size());
            ImGui::Separator();
            
            static int selected = -1;
            
            for (int i = 0; i < history.size(); i++) {
                char label[128];
                snprintf(label, sizeof(label), "[%d] %s", i + 1, history[i].timestamp.c_str());
                
                if (ImGui::Selectable(label, selected == i)) {
                    selected = i;
                    current_display_location = history[i];
                }
            }
            
            ImGui::Separator();
            
            if (ImGui::Button("Close")) {
                ImGui::CloseCurrentPopup();
            }
            
            ImGui::EndPopup();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Clear Display")) {
            current_display_location = LocationData{};
        }
        
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), 
                          "Server Status: RUNNING");
        ImGui::Text("Listening on port: 5556");
        
        ImGui::End();
        
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(window);
    }
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "[GUI] Window closed" << std::endl;
    g_server_running = false;
}

int main() {
    std::cout << "Starting Location Server with GUI..." << std::endl;
    
    std::thread zmq_thread(zmq_server_thread);
    
    gui_thread();
    
    if (zmq_thread.joinable()) {
        zmq_thread.join();
    }
    
    std::cout << "Server shutdown complete" << std::endl;
    return 0;
}