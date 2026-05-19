#include "app_gui.h"
#include "app_utils.h"
#include <GL/glew.h>
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_opengl3.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <algorithm>

using namespace std;

void run_gui_thread(SystemStatus* shared_data, Database* db, DatabaseQueue* dbQueue, atomic<bool>* should_stop) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return;
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* window = SDL_CreateWindow("Backend Control", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1400, 900, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    SDL_GLContext gl = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl);
    SDL_GL_SetSwapInterval(1);
    if (glewInit() != GLEW_OK) return;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    
    // ===== FONT: JetBrains Mono =====
    // Download: https://www.jetbrains.com/lp/mono/
    // Place JetBrainsMono-Regular.ttf into fonts/ folder
    io.Fonts->Clear();
    ImFontConfig fc;
    fc.OversampleH = 2;
    fc.OversampleV = 2;
    ImFont* font = nullptr;
#ifdef _WIN32
    font = io.Fonts->AddFontFromFileTTF("fonts/jetbrains.ttf", 17.0f, &fc, io.Fonts->GetGlyphRangesCyrillic());
    if (!font) font = io.Fonts->AddFontFromFileTTF("jetbrains.ttf", 17.0f, &fc, io.Fonts->GetGlyphRangesCyrillic());
#endif
    if (!font) font = io.Fonts->AddFontDefault(&fc);
    io.Fonts->Build();
    // ================================
    
    // ===== COLOR PALETTE (dark blue theme) =====
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_WindowBg]         = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_TitleBg]          = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]    = ImVec4(0.14f, 0.15f, 0.17f, 1.00f);
    colors[ImGuiCol_FrameBg]          = ImVec4(0.15f, 0.16f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.20f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.25f, 0.27f, 0.30f, 1.00f);
    colors[ImGuiCol_Button]           = ImVec4(0.18f, 0.40f, 0.65f, 1.00f);
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.22f, 0.48f, 0.78f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.14f, 0.34f, 0.56f, 1.00f);
    colors[ImGuiCol_Header]           = ImVec4(0.18f, 0.40f, 0.65f, 0.60f);
    colors[ImGuiCol_HeaderHovered]    = ImVec4(0.22f, 0.48f, 0.78f, 0.80f);
    colors[ImGuiCol_HeaderActive]     = ImVec4(0.14f, 0.34f, 0.56f, 1.00f);
    colors[ImGuiCol_Tab]              = ImVec4(0.12f, 0.13f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered]       = ImVec4(0.22f, 0.48f, 0.78f, 0.80f);
    colors[ImGuiCol_TabActive]        = ImVec4(0.18f, 0.40f, 0.65f, 1.00f);
    colors[ImGuiCol_CheckMark]        = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrab]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.30f, 0.64f, 1.00f, 1.00f);
    colors[ImGuiCol_Text]             = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
    colors[ImGuiCol_TextDisabled]     = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_Separator]        = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_ResizeGrip]       = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
    // ===========================================
    
    // Rounded corners
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 3.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);
    
    ImGui_ImplSDL2_InitForOpenGL(window, gl);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    TileManager tileManager("tiles");
    MapRenderer mapRenderer(tileManager);
    
    int signalGraphTab = 0;
    float timeWindowSeconds = 120.0f;
    auto lastGuiUpdate = chrono::steady_clock::now();
    
    while (!should_stop->load()) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) should_stop->store(true);
        }
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Backend Control Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        
        // --- STATUS BAR ---
        {
            lock_guard<mutex> lock(shared_data->mtx);
            long long nowEpoch = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
            long long age = nowEpoch - shared_data->lastMessageTime.load();
            
            ImGui::BeginChild("StatusBar", ImVec2(0, 36), true, ImGuiWindowFlags_NoScrollbar);
            if (shared_data->has_connection && age < 5000)
                ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.30f, 1.00f), "ANDROID: ONLINE");
            else
                ImGui::TextColored(ImVec4(1.00f, 0.85f, 0.20f, 1.00f), "ANDROID: DELAY (%lld ms)", age);
            ImGui::SameLine(180);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.00f), "MESSAGES: %d", shared_data->messageCount.load());
            ImGui::SameLine(380);
            if (db->isConnected())
                ImGui::TextColored(ImVec4(0.20f, 0.90f, 0.30f, 1.00f), "DATABASE: CONNECTED");
            else
                ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.25f, 1.00f), "DATABASE: DISCONNECTED");
            ImGui::SameLine(620);
            ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.65f, 1.00f), "QUEUE: %d", shared_data->dbPendingTasks.load());
            ImGui::EndChild();
        }
        
        ImGui::Spacing();
        
        // --- TABS ---
        if (ImGui::BeginTabBar("MainTabs")) {
            
            // ===== TAB: LOCATION =====
            if (ImGui::BeginTabItem("LOCATION")) {
                lock_guard<mutex> lock(shared_data->mtx);
                ImGui::BeginChild("LocInfo", ImVec2(0, 140), true);
                ImGui::Columns(2, nullptr, false);
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Latitude:"); ImGui::NextColumn();
                ImGui::Text("%.6f", shared_data->lat); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Longitude:"); ImGui::NextColumn();
                ImGui::Text("%.6f", shared_data->lon); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Altitude:"); ImGui::NextColumn();
                ImGui::Text("%.2f m", shared_data->alt); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "GPS Accuracy:"); ImGui::NextColumn();
                ImGui::Text("%.1f m", shared_data->accuracy); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::EndChild();
                if (shared_data->timestamp > 0)
                    ImGui::TextColored(ImVec4(0.50f, 0.80f, 0.50f, 1.00f), "Last update: %s", formatTimestamp(shared_data->timestamp).c_str());
                ImGui::EndTabItem();
            }
            
            // ===== TAB: SIGNAL GRAPHS =====
            if (ImGui::BeginTabItem("SIGNAL GRAPHS")) {
                lock_guard<mutex> lock(shared_data->mtx);
                
                ImGui::BeginChild("GraphControls", ImVec2(0, 45), true);
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 0));
                ImGui::Text("TYPE:"); ImGui::SameLine();
                if (ImGui::Button("RSRP", ImVec2(80, 28))) signalGraphTab = 0;
                ImGui::SameLine();
                if (ImGui::Button("RSSI", ImVec2(80, 28))) signalGraphTab = 1;
                ImGui::SameLine();
                if (ImGui::Button("SINR", ImVec2(80, 28))) signalGraphTab = 2;
                ImGui::SameLine(350);
                ImGui::Text("WINDOW:"); ImGui::SameLine();
                ImGui::PushItemWidth(180);
                ImGui::SliderFloat("##TimeWin", &timeWindowSeconds, 30.0f, 300.0f, "%.0f sec");
                ImGui::PopItemWidth();
                ImGui::PopStyleVar();
                ImGui::EndChild();
                
                ImGui::Spacing();
                
                switch (signalGraphTab) {
                    case 0: renderPciSignalGraph(shared_data->pciSignalHistory, "RSRP (Reference Signal Received Power)", [](const SignalPoint& p) { return static_cast<double>(p.rsrp); }, "RSRP [dBm]", -140.0, -40.0); break;
                    case 1: renderPciSignalGraph(shared_data->pciSignalHistory, "RSSI (Received Signal Strength Indicator)", [](const SignalPoint& p) { return static_cast<double>(p.rssi); }, "RSSI [dBm]", -140.0, -40.0); break;
                    case 2: renderPciSignalGraph(shared_data->pciSignalHistory, "SINR (Signal to Interference plus Noise Ratio)", [](const SignalPoint& p) { return static_cast<double>(p.sinr); }, "SINR [dB]", -20.0, 40.0); break;
                }
                ImGui::EndTabItem();
            }
            
            // ===== TAB: MAP =====
            if (ImGui::BeginTabItem("MAP")) {
                lock_guard<mutex> lock(shared_data->mtx);
                MapState& map = shared_data->map;
                
                ImGui::BeginChild("MapToolbar", ImVec2(0, 40), true);
                ImGui::Text("ZOOM: %d", map.zoom); ImGui::SameLine();
                if (ImGui::Button("-", ImVec2(28, 26))) { 
                    map.zoom = max(1, map.zoom - 1); 
                    tileManager.clearQueue();
                    tileManager.clearCache();
                }
                ImGui::SameLine();
                if (ImGui::Button("+", ImVec2(28, 26))) { 
                    map.zoom = min(19, map.zoom + 1); 
                    tileManager.clearQueue();
                    tileManager.clearCache();
                }
                ImGui::SameLine(150);
                if (ImGui::Button("CENTER ON GPS", ImVec2(140, 26))) {
                    if (shared_data->lat != 0.0 || shared_data->lon != 0.0) { 
                        map.centerLat = shared_data->lat; 
                        map.centerLon = shared_data->lon;
                        tileManager.clearQueue();
                    }
                }
                ImGui::SameLine(310);
                if (ImGui::Button("CLEAR CACHE", ImVec2(120, 26))) tileManager.clearCache();
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "  Center: %.5f, %.5f  |  Tiles: %zu", map.centerLat, map.centerLon, tileManager.getCacheSize());
                ImGui::EndChild();
                
                ImVec2 mapSize = ImGui::GetContentRegionAvail();
                mapSize.x = max(500.0f, min(mapSize.x, 1100.0f));
                mapSize.y = max(350.0f, min(mapSize.y, 650.0f));
                
                ImGui::BeginChild("MapView", mapSize, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
                ImVec2 mapPos = ImGui::GetCursorScreenPos();
                mapRenderer.handleInput(map, mapPos, mapSize);
                mapRenderer.render(map, *shared_data, mapSize);
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            // ===== TAB: NETWORK =====
            if (ImGui::BeginTabItem("NETWORK")) {
                lock_guard<mutex> lock(shared_data->mtx);
                ImGui::BeginChild("NetInfo", ImVec2(0, 60), true);
                ImGui::Columns(2, nullptr, false);
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Bytes Sent:"); ImGui::NextColumn();
                ImGui::Text("%s", formatBytes(shared_data->netSent).c_str()); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Bytes Received:"); ImGui::NextColumn();
                ImGui::Text("%s", formatBytes(shared_data->netRecv).c_str()); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::EndChild();
                
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.26f, 0.59f, 0.98f, 1.00f), "Top Applications:");
                ImGui::BeginChild("TopApps", ImVec2(0, 250), true);
                if (shared_data->topApps.empty())
                    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.00f), "No data available");
                else {
                    ImGui::Columns(3, nullptr, false);
                    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.00f), "#"); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.00f), "Package"); ImGui::NextColumn();
                    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.00f), "Traffic"); ImGui::NextColumn();
                    ImGui::Separator();
                    for (size_t i = 0; i < shared_data->topApps.size() && i < 10; ++i) {
                        ImGui::Text("%2zu", i + 1); ImGui::NextColumn();
                        ImGui::Text("%s", shared_data->topApps[i].packageName.c_str()); ImGui::NextColumn();
                        ImGui::Text("%s", formatBytes(shared_data->topApps[i].bytes).c_str()); ImGui::NextColumn();
                    }
                    ImGui::Columns(1);
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            // ===== TAB: TELEPHONY =====
            if (ImGui::BeginTabItem("TELEPHONY")) {
                lock_guard<mutex> lock(shared_data->mtx);
                ImGui::BeginChild("TeleInfo", ImVec2(0, 50), true);
                ImGui::TextWrapped("%s", shared_data->telephonySummary.c_str());
                ImGui::EndChild();
                
                ImGui::Spacing();
                if (shared_data->cells.empty())
                    ImGui::TextColored(ImVec4(0.50f, 0.50f, 0.50f, 1.00f), "No cell towers detected");
                else {
                    ImGui::TextColored(ImVec4(0.26f, 0.59f, 0.98f, 1.00f), "Cell Towers: %zu", shared_data->cells.size());
                    ImGui::BeginChild("CellList", ImVec2(0, 350), true);
                    for (size_t i = 0; i < shared_data->cells.size() && i < 15; ++i) {
                        const auto& c = shared_data->cells[i];
                        ImGui::PushID(static_cast<int>(i));
                        string hdr = "[" + to_string(i + 1) + "] " + c.type;
                        if (c.pci != -1) hdr += " (PCI=" + to_string(c.pci) + ")";
                        else if (!c.nci.empty() && c.nci != " ") hdr += " (NCI=" + c.nci + ")";
                        if (ImGui::CollapsingHeader(hdr.c_str())) {
                            ImGui::Indent(); renderCellDetails(c); ImGui::Unindent();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndChild();
                }
                ImGui::EndTabItem();
            }
            
            // ===== TAB: LOCAL DATA =====
            if (ImGui::BeginTabItem("LOCAL DATA")) {
                lock_guard<mutex> lock(shared_data->mtx);
                renderSignalGraph(shared_data->localSignalHistory, "RSRP from JSON", [](const SignalPoint& p) { return static_cast<double>(p.rsrp); }, "RSRP (dBm)", -140.0, -40.0);
                ImGui::Separator();
                ImGui::BeginChild("GPSLocal", ImVec2(-1, 300), true);
                if (ImPlot::BeginPlot("##GPSLocal", ImVec2(-1, -1))) {
                    ImPlot::SetupAxes("Longitude", "Latitude");
                    ImPlot::SetupAxisFormat(ImAxis_X1, "%.6f");
                    ImPlot::SetupAxisFormat(ImAxis_Y1, "%.6f");
                    auto xy = shared_data->localLocationHistory.getPlotData();
                    const auto& lats = xy.first;
                    const auto& lons = xy.second;
                    if (!lats.empty() && !lons.empty()) {
                        int count = static_cast<int>(lats.size());
                        ImPlot::PlotLine("Track", lons.data(), lats.data(), count);
                        if (count < 5000) { ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f); ImPlot::PlotScatter("Points", lons.data(), lats.data(), count); }
                    } else ImPlot::PlotText("No data", 0, 0);
                    ImPlot::EndPlot();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            
            // ===== TAB: SETTINGS =====
            if (ImGui::BeginTabItem("SETTINGS")) {
                lock_guard<mutex> lock(shared_data->mtx);
                ImGui::BeginChild("FilterGroup", ImVec2(0, 100), true);
                ImGui::TextColored(ImVec4(0.26f, 0.59f, 0.98f, 1.00f), "Data Filters:");
                ImGui::Spacing();
                ImGui::Checkbox("Location", &shared_data->filterLocation);
                ImGui::SameLine(200);
                ImGui::Checkbox("Telephony", &shared_data->filterTelephony);
                ImGui::SameLine(400);
                ImGui::Checkbox("Network", &shared_data->filterNetwork);
                ImGui::EndChild();
                
                ImGui::Spacing();
                ImGui::BeginChild("StatsGroup", ImVec2(0, 100), true);
                ImGui::TextColored(ImVec4(0.26f, 0.59f, 0.98f, 1.00f), "Statistics:");
                ImGui::Spacing();
                ImGui::Columns(2, nullptr, false);
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "DB Records:"); ImGui::NextColumn();
                ImGui::Text("%d", shared_data->dbInsertCount.load()); ImGui::NextColumn();
                ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.60f, 1.00f), "Messages/sec:"); ImGui::NextColumn();
                ImGui::Text("%d", shared_data->messagesPerSecond.load()); ImGui::NextColumn();
                ImGui::Columns(1);
                ImGui::EndChild();
                
                ImGui::Spacing();
                if (ImGui::Button("RESET HISTORY", ImVec2(200, 32))) { 
                    shared_data->signalHistory.reset(); 
                    shared_data->pciSignalHistory.clear(); 
                }
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
        
        ImGui::End();
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(0.08f, 0.09f, 0.10f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
        auto now = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - lastGuiUpdate).count();
        if (elapsed < 16) this_thread::sleep_for(chrono::milliseconds(16 - elapsed));
        lastGuiUpdate = now;
    }
    tileManager.stop();
    ImPlot::DestroyContext();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(window);
    SDL_Quit();
}