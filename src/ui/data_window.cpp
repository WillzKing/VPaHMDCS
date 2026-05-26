#include "ui/ui_components.h"
#include <imgui.h>
#include <chrono>
#include <iomanip>
#include <sstream>

void DrawDataWindow(bool& open, UserData& user, std::mutex& mtx) {
    if (!open) return;
    if (ImGui::Begin("Data Information", &open)) {
        std::lock_guard<std::mutex> lock(mtx);
        
        ImGui::TextColored(ImVec4(0.22f, 0.55f, 0.90f, 1.00f), "User: %s", user.user.c_str());
        ImGui::Separator();
        
        ImGui::Columns(2, nullptr, false);
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.00f), "Latitude"); ImGui::NextColumn();
        ImGui::Text("%.6f", user.location.latitude); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.00f), "Longitude"); ImGui::NextColumn();
        ImGui::Text("%.6f", user.location.longitude); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.00f), "Altitude"); ImGui::NextColumn();
        ImGui::Text("%.2f m", user.location.altitude); ImGui::NextColumn();
        ImGui::TextColored(ImVec4(0.60f, 0.60f, 0.62f, 1.00f), "Accuracy"); ImGui::NextColumn();
        ImGui::Text("%.2f m", user.location.accuracy); ImGui::NextColumn();
        ImGui::Columns(1);
        
        if (user.location.timestamp > 0) {
            auto tp_ms = std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>(
                std::chrono::milliseconds(user.location.timestamp));
            std::time_t time_t_tp = std::chrono::system_clock::to_time_t(tp_ms);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp_ms.time_since_epoch()) % 1000;
            
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t_tp), "%Y-%m-%d %H:%M:%S");
            ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
            ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1.00f), "Timestamp: %s", ss.str().c_str());
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.22f, 0.55f, 0.90f, 1.00f), "Active Cells");
        for (const auto& n : user.mobileNetworks) {
            ImGui::BulletText("%s | PCI:%d TAC:%d RSRP:%d RSRQ:%d RSSI:%d", 
                n.cellIdentity.c_str(), n.pci, n.tac, n.rsrp, n.rsrq, n.rssi);
        }
    }
    ImGui::End();
}