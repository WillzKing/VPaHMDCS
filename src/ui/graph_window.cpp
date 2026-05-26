#include "ui/ui_components.h"
#include <imgui.h>
#include <implot.h>
#include <algorithm>
#include <limits>

using ValVec = std::vector<double>;
using Getter = ValVec& (*)(SignalData&);

static ValVec& GetRsrp(SignalData& s) { return s.rsrp_values; }
static ValVec& GetRsrq(SignalData& s) { return s.rsrq_values; }
static ValVec& GetRssi(SignalData& s) { return s.rssi_values; }

static const struct { const char* title, *yAxis; Getter getter; double yMin, yMax; } kPlots[] = {
    {"RSRP", "dBm", GetRsrp, -140, -40},
    {"RSRQ", "dB",  GetRsrq,  -40,   0},
    {"RSSI", "dBm", GetRssi, -130, -20},
};

static void SortedSeries(const SignalData& s, const ValVec& vals,
                         std::vector<double>& t_out, std::vector<double>& v_out) {
    if (vals.empty() || s.timestamps.size() != vals.size()) return;
    
    std::vector<std::pair<double,double>> tv(vals.size());
    for (size_t i = 0; i < vals.size(); ++i) tv[i] = {s.timestamps[i], vals[i]};
    
    std::sort(tv.begin(), tv.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
    
    const double t0 = tv.front().first;
    const double kMaxGapSeconds = 600.0;

    t_out.clear(); v_out.clear();
    for (size_t i = 0; i < tv.size(); ++i) { 
        double t_curr = (tv[i].first - t0) / 1000.0;

        if (i > 0) {
            double t_prev = (tv[i-1].first - t0) / 1000.0;
            if (t_curr - t_prev > kMaxGapSeconds) {
                t_out.push_back(std::numeric_limits<double>::quiet_NaN());
                v_out.push_back(std::numeric_limits<double>::quiet_NaN());
            }
        }

        t_out.push_back(t_curr);
        v_out.push_back(tv[i].second);
    }
}

void DrawGraphWindow(bool& open, std::vector<SignalData>& signals, std::string& selectedIdentity, std::mutex& mtx)
{
    if (!open) return;
    if (!ImGui::Begin("Signal Graphs", &open)) { ImGui::End(); return; }

    std::lock_guard<std::mutex> lock(mtx);
    
    if (signals.empty()) {
        ImGui::TextColored(ImVec4(0.55f, 0.55f, 0.58f, 1.00f), "No data available. Connect Android device to see signal graphs.");
        ImGui::End();
        return;
    }
    
    if (selectedIdentity.empty()) selectedIdentity = signals[0].cellIdentity;

    ImGui::TextColored(ImVec4(0.22f, 0.55f, 0.90f, 1.00f), "Selected Cell:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300);
    if (ImGui::BeginCombo("##CellSelector", selectedIdentity.c_str())) {
        for (const auto& s : signals) {
            bool sel = (selectedIdentity == s.cellIdentity);
            if (ImGui::Selectable(s.cellIdentity.c_str(), sel)) selectedIdentity = s.cellIdentity;
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.50f, 0.52f, 0.55f, 1.00f), "Series: %d", (int)signals.size());

    const float plotH = std::max(220.0f, ImGui::GetContentRegionAvail().y / 3.0f);

    for (const auto& pm : kPlots) {
        if (!ImPlot::BeginPlot(pm.title, ImVec2(-1, plotH))) continue;
        ImPlot::SetupAxes("Time (s)", pm.yAxis);
        ImPlot::SetupAxisLimits(ImAxis_Y1, pm.yMin, pm.yMax, ImPlotCond_Once);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);
        
        for (auto& s : signals) {
            std::vector<double> t, v;
            SortedSeries(s, pm.getter(s), t, v);
            
            if (!t.empty()) {
                ImPlot::PlotLine(s.cellIdentity.c_str(), t.data(), v.data(), (int)t.size());
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 2.0f);
                ImPlot::PlotScatter(s.cellIdentity.c_str(), t.data(), v.data(), (int)t.size());
            }
        }
        ImPlot::EndPlot();
    }
    ImGui::End();
}