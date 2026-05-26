#include "gui_app.h"
#include "gui_tabs.h"
#include "ui/ui_widget.h"

#include <SDL3/SDL.h>
#include <GL/glew.h>
#include <imgui.h>
#include <implot.h>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_opengl3.h>
#include <filesystem>

static SDL_Window* s_window = nullptr;
static SDL_GLContext s_gl     = nullptr;

bool GuiAppInit(int width, int height, const char* title) {
    if (!SDL_Init(SDL_INIT_VIDEO)) return false;

    s_window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!s_window) return false;

    s_gl = SDL_GL_CreateContext(s_window);
    SDL_GL_MakeCurrent(s_window, s_gl);
    SDL_GL_SetSwapInterval(1);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL3_InitForOpenGL(s_window, s_gl);
    ImGui_ImplOpenGL3_Init("#version 330");

    // JetBrains Mono font loading with fallback
    const char* fontPaths[] = {"fonts/jetbrains.ttf", "../fonts/jetbrains.ttf", "jetbrains.ttf"};
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    bool fontLoaded = false;
    for (auto* p : fontPaths) {
        if (std::filesystem::exists(p)) {
            io.Fonts->AddFontFromFileTTF(p, 18.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());
            fontLoaded = true;
            break;
        }
    }
    if (!fontLoaded) {
        io.Fonts->AddFontDefault();
    }

    SetupAmethystStyle();
    return true;
}

bool GuiAppFrame(RuntimeState& state) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        ImGui_ImplSDL3_ProcessEvent(&e);
        if (e.type == SDL_EVENT_QUIT) state.running.store(false);
    }
    if (!state.running.load()) return false;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos({0, 0}, ImGuiCond_Always);
    ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
    ImGui::Begin("##root", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);

    // Status Bar
     {
        ImGui::BeginChild("StatusBar", ImVec2(0, 38), true, ImGuiWindowFlags_NoScrollbar);
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.68f, 1.00f), "Status:");
        ImGui::SameLine(80);
        if (state.packets.load() > 0) {
            ImGui::TextColored(ImVec4(0.20f, 0.88f, 0.30f, 1.00f), "Connected");
        } else {
            ImGui::TextColored(ImVec4(0.95f, 0.25f, 0.25f, 1.00f), "Waiting for connection");
        }
        ImGui::SameLine(400);
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.68f, 1.00f), "Packets: %d", state.packets.load());
        ImGui::SameLine(600);
        ImGui::TextColored(ImVec4(0.65f, 0.65f, 0.68f, 1.00f), "RX: %.1f KB", state.bytesRx.load() / 1024.0);
        ImGui::EndChild();
    }

    ImGui::Spacing();

    if (ImGui::BeginTabBar("MainTabs")) {
        if (ImGui::BeginTabItem("Main"))       { TabMain(state);      ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Graphs"))     { TabSignals(state);   ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Local Data")) { TabLocal(state);     ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Map"))        { TabMap(state);       ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Telephony"))  { TabTelephony(state); ImGui::EndTabItem(); }
        if (ImGui::BeginTabItem("Filters"))    { TabFilters(state);   ImGui::EndTabItem(); }
        ImGui::EndTabBar();
    }
    ImGui::End();

    ImGui::Render();
    glClearColor(0.08f, 0.09f, 0.10f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(s_window);
    return true;
}

void GuiAppShutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DestroyContext(s_gl);
    SDL_DestroyWindow(s_window);
    SDL_Quit();
}