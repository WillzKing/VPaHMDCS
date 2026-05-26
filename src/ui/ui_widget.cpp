#include "ui/ui_widget.h"
#include "ui/ui_components.h"
#include <imgui.h>

void SetupAmethystStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    
    // Spacing & Sizing
    s.WindowPadding    = ImVec2(10.0f, 10.0f);
    s.FramePadding     = ImVec2(8.0f, 5.0f);
    s.CellPadding      = ImVec2(8.0f, 5.0f);
    s.ItemSpacing      = ImVec2(10.0f, 8.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    s.ScrollbarSize    = 14.0f;
    s.GrabMinSize      = 12.0f;

    // Borders & Rounding
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize  = 1.0f;
    s.PopupBorderSize  = 1.0f;
    s.FrameBorderSize  = 1.0f;
    s.WindowRounding   = 8.0f;
    s.ChildRounding    = 6.0f;
    s.FrameRounding    = 6.0f;
    s.PopupRounding    = 5.0f;
    s.ScrollbarRounding = 8.0f;
    s.GrabRounding     = 4.0f;
    s.TabRounding      = 6.0f;

    // Colors: Dark Professional with Blue Accent
    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text]                  = ImVec4(0.90f, 0.90f, 0.92f, 1.00f);
    colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.52f, 0.55f, 1.00f);
    colors[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.09f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.08f, 0.09f, 0.10f, 0.96f);
    colors[ImGuiCol_Border]               = ImVec4(0.20f, 0.22f, 0.25f, 0.70f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]              = ImVec4(0.14f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.20f, 0.22f, 0.26f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.24f, 0.26f, 0.30f, 1.00f);
    colors[ImGuiCol_TitleBg]              = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.13f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.06f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_MenuBarBg]            = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.06f, 0.07f, 0.09f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.32f, 0.38f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.38f, 0.40f, 0.48f, 1.00f);
    colors[ImGuiCol_CheckMark]            = ImVec4(0.22f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrab]           = ImVec4(0.22f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.28f, 0.62f, 0.95f, 1.00f);
    colors[ImGuiCol_Button]               = ImVec4(0.16f, 0.38f, 0.60f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.20f, 0.46f, 0.72f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.12f, 0.32f, 0.52f, 1.00f);
    colors[ImGuiCol_Header]               = ImVec4(0.16f, 0.38f, 0.60f, 0.55f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.20f, 0.46f, 0.72f, 0.75f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.12f, 0.32f, 0.52f, 1.00f);
    colors[ImGuiCol_Separator]            = ImVec4(0.18f, 0.20f, 0.23f, 1.00f);
    colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.22f, 0.55f, 0.90f, 0.78f);
    colors[ImGuiCol_SeparatorActive]      = ImVec4(0.22f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_Tab]                  = ImVec4(0.11f, 0.12f, 0.15f, 1.00f);
    colors[ImGuiCol_TabHovered]           = ImVec4(0.20f, 0.46f, 0.72f, 0.75f);
    colors[ImGuiCol_TabActive]            = ImVec4(0.16f, 0.38f, 0.60f, 1.00f);
    colors[ImGuiCol_TabUnfocused]         = ImVec4(0.08f, 0.09f, 0.11f, 1.00f);
    colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.12f, 0.14f, 0.18f, 1.00f);
    colors[ImGuiCol_TableHeaderBg]        = ImVec4(0.14f, 0.16f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong]    = ImVec4(0.22f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_TableBorderLight]     = ImVec4(0.16f, 0.18f, 0.22f, 1.00f);
    colors[ImGuiCol_TableRowBg]           = ImVec4(0.10f, 0.11f, 0.13f, 1.00f);
    colors[ImGuiCol_TableRowBgAlt]        = ImVec4(1.00f, 1.00f, 1.00f, 0.03f);
    colors[ImGuiCol_TextSelectedBg]       = ImVec4(0.22f, 0.55f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget]       = ImVec4(0.22f, 0.55f, 0.90f, 0.95f);
    colors[ImGuiCol_NavHighlight]         = ImVec4(0.22f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]    = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]     = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}

void CreateMobileNetworkWidget(UserData& currentUser, std::mutex& dataMutex, std::vector<SignalData>& signalData) {
    static UIState state;
    DrawMenuBar(state);
    DrawDataWindow(state.showDataWindow, currentUser, dataMutex);
    DrawGraphWindow(state.showGraphWindow, signalData, state.selectedCellIdentity, dataMutex);
}