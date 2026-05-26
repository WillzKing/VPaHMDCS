#include "ui/ui_components.h"
#include <imgui.h>

void DrawMenuBar(UIState& state)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Views"))
        {
            ImGui::MenuItem("Data Information", nullptr, &state.showDataWindow);
            ImGui::MenuItem("Signal Graphs", nullptr, &state.showGraphWindow);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
