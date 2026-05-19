#include "app_utils.h"
#include <ctime>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

string formatTimestamp(long long ts) {
    if (ts <= 0) return "N/A";
    time_t raw = ts / 1000;
    char buf[80];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&raw));
    return string(buf);
}

string formatBytes(long long bytes) {
    const char* units[] = {"Б", "КБ", "МБ", "ГБ", "ТБ"};
    int i = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && i < 4) { size /= 1024.0; i++; }
    char buf[64];
    snprintf(buf, sizeof(buf), i == 0 ? "%.0f %s" : "%.2f %s", size, units[i]);
    return string(buf);
}

string formatRSRP(int rsrp) {
    if (rsrp <= -140) return "Нет сигнала";
    if (rsrp >= -80) return "Отлично";
    if (rsrp >= -90) return "Хорошо";
    if (rsrp >= -100) return "Средне";
    if (rsrp >= -110) return "Плохо";
    return "Очень плохо";
}

ImVec4 getRSRPColor(int rsrp) {
    if (rsrp >= -80) return ImVec4(0, 1, 0, 1);
    if (rsrp >= -90) return ImVec4(0.5f, 1, 0, 1);
    if (rsrp >= -100) return ImVec4(1, 1, 0, 1);
    if (rsrp >= -110) return ImVec4(1, 0.5f, 0, 1);
    return ImVec4(1, 0, 0, 1);
}

string get_string_safe(const json& val) {
    try {
        if (val.is_null()) return " ";
        if (val.is_string()) { string s = val.get<string>(); return s.empty() ? " " : s; }
        if (val.is_number_integer()) return to_string(val.get<int64_t>());
        return val.dump();
    } catch (...) { return " "; }
}

string get_string_safe(const json& obj, const string& key) {
    try {
        if (!obj.is_object() || !obj.contains(key)) return " ";
        return get_string_safe(obj[key]);
    } catch (...) { return " "; }
}

int get_int_safe(const json& obj, const string& key, int defaultVal) {
    try {
        if (!obj.is_object() || !obj.contains(key) || obj[key].is_null()) return defaultVal;
        return obj[key].get<int>();
    } catch (...) { return defaultVal; }
}

void renderCellDetails(const CellInfo& c) {
    ImGui::BulletText("MCC: %s | MNC: %s", c.mcc.c_str(), c.mnc.c_str());
    ImGui::BulletText("Cell ID: %s | LAC/TAC: %s/%s", c.cellId.c_str(), c.lac.c_str(), c.tac.c_str());
    if (c.type == "LTE") {
        if (c.pci != -1) ImGui::BulletText("PCI: %d", c.pci);
        if (c.earfcn != -1) ImGui::BulletText("EARFCN: %d", c.earfcn);
        if (c.band != -1) ImGui::BulletText("Band: %d", c.band);
        ImGui::BulletText("RSRP: %d дБм (%s)", c.rsrp, formatRSRP(c.rsrp).c_str());
        ImGui::BulletText("RSSI: %d дБм | RSRQ: %d дБ", c.rssi, c.rsrq);
        ImGui::BulletText("SINR/RSSNR: %d дБ", c.sinr);
        ImGui::BulletText("CQI: %d | ASU: %d", c.cqi, c.asuLevel);
        ImGui::BulletText("Timing Advance: %d", c.timingAdvance);
    }
    else if (c.type == "GSM") {
        if (c.arfcn != -1) ImGui::BulletText("ARFCN: %d", c.arfcn);
        if (c.bsic != -1) ImGui::BulletText("BSIC: %d", c.bsic);
        if (c.psc != -1) ImGui::BulletText("PSC: %d", c.psc);
        ImGui::BulletText("RSSI/DBM: %d дБм (%s)", c.rssi, formatRSRP(c.rssi).c_str());
        ImGui::BulletText("ASU: %d", c.asuLevel);
        ImGui::BulletText("Timing Advance: %d", c.timingAdvance);
    }
    else if (c.type == "5G_NR") {
        if (c.pci != -1) ImGui::BulletText("PCI: %d", c.pci);
        if (!c.nci.empty() && c.nci != " ") ImGui::BulletText("NCI: %s", c.nci.c_str());
        if (c.nrarfcn != -1) ImGui::BulletText("NR-ARFCN: %d", c.nrarfcn);
        if (c.band != -1) ImGui::BulletText("Band: %d", c.band);
        ImGui::BulletText("SS-RSRP: %d дБм (%s)", c.ssRsrp, formatRSRP(c.ssRsrp).c_str());
        ImGui::BulletText("SS-RSRQ: %d дБ | SS-SINR: %d дБ", c.ssRsrq, c.ssSinr);
        ImGui::BulletText("ASU: %d", c.asuLevel);
        ImGui::BulletText("Timing Advance: %d", c.timingAdvance);
    }
    else {
        if (c.pci != -1) ImGui::BulletText("PCI: %d", c.pci);
        if (c.earfcn != -1) ImGui::BulletText("EARFCN: %d", c.earfcn);
        if (c.nrarfcn != -1) ImGui::BulletText("NR-ARFCN: %d", c.nrarfcn);
        ImGui::BulletText("RSRP: %d дБм (%s)", c.rsrp, formatRSRP(c.rsrp).c_str());
        ImGui::BulletText("RSSI: %d дБм | RSRQ: %d дБ", c.rssi, c.rsrq);
        ImGui::BulletText("SINR: %d дБ", c.sinr);
        ImGui::BulletText("CQI: %d | ASU: %d", c.cqi, c.asuLevel);
        ImGui::BulletText("Timing Advance: %d", c.timingAdvance);
    }
}

static const ImVec4 PCI_COLORS[] = {
    ImVec4(1.0f, 0.0f, 0.0f, 1.0f), ImVec4(0.0f, 1.0f, 0.0f, 1.0f), ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
    ImVec4(1.0f, 1.0f, 0.0f, 1.0f), ImVec4(1.0f, 0.0f, 1.0f, 1.0f), ImVec4(0.0f, 1.0f, 1.0f, 1.0f),
    ImVec4(1.0f, 0.5f, 0.0f, 1.0f), ImVec4(0.5f, 0.0f, 1.0f, 1.0f), ImVec4(0.0f, 1.0f, 0.5f, 1.0f),
    ImVec4(1.0f, 0.0f, 0.5f, 1.0f)
};

ImVec4 getPciColor(int pci) {
    if (pci < 0) return ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
    return PCI_COLORS[pci % (sizeof(PCI_COLORS) / sizeof(PCI_COLORS[0]))];
}

void renderPciSignalGraph(PciSignalHistory& history, const string& title,
                          const function<double(const SignalPoint&)>& getValue,
                          const string& yAxisLabel, double yMin, double yMax) {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", title.c_str());
    ImGui::BeginChild(title.c_str(), ImVec2(-1, 400), true);
    auto pcis = history.getUniquePCIs();
    if (pcis.empty()) {
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Ожидание данных...");
        ImGui::EndChild();
        return;
    }
    if (ImPlot::BeginPlot(("##" + title).c_str(), ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Время (с)", yAxisLabel.c_str());
        ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax, ImPlotCond_Always);
        ImPlot::SetupAxisFormat(ImAxis_Y1, "%.0f");
        ImPlot::SetupMouseText(ImPlotLocation_SouthEast);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal | ImPlotLegendFlags_Outside);
        for (int pci : pcis) {
            auto xy = history.getGraphData(pci, getValue);
            const auto& xValues = xy.first;
            const auto& yValues = xy.second;
            if (!xValues.empty() && !yValues.empty()) {
                ImVec4 color = getPciColor(pci);
                ImPlot::PushStyleColor(ImPlotCol_Line, color);
                string label = "PCI-" + to_string(pci);
                ImPlot::PlotLine(label.c_str(), xValues.data(), yValues.data(), static_cast<int>(xValues.size()));
                ImPlot::PopStyleColor();
            }
        }
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
    ImGui::Text("Всего PCI: %zu | Точек: %zu", pcis.size(), history.getTotalPoints());
}

void renderSignalGraph(SignalHistory& history, const string& title,
                       const function<double(const SignalPoint&)>& getValue,
                       const string& yAxisLabel, double yMin, double yMax) {
    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", title.c_str());
    ImGui::BeginChild(title.c_str(), ImVec2(-1, 350), true);
    if (ImPlot::BeginPlot(("##" + title).c_str(), ImVec2(-1, -1))) {
        ImPlot::SetupAxes("Время (с)", yAxisLabel.c_str());
        ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax, ImPlotCond_Always);
        ImPlot::SetupMouseText(ImPlotLocation_SouthEast);
        ImPlot::SetupLegend(ImPlotLocation_NorthWest, ImPlotLegendFlags_Horizontal);
        auto series = history.getPlotData(getValue);
        int cnt = 0;
        for (const auto& kv : series) {
            if (cnt >= 30) break;
            const auto& data = kv.second;
            const auto& x = data.first;
            const auto& y = data.second;
            if (!x.empty() && !y.empty()) ImPlot::PlotLine(kv.first.c_str(), x.data(), y.data(), static_cast<int>(x.size()));
            cnt++;
        }
        if (cnt == 0) ImPlot::PlotText("Ожидание данных...", 0, (yMin + yMax) / 2);
        ImPlot::EndPlot();
    }
    ImGui::EndChild();
    ImGui::Text("Серий: %zu | Точек: %zu", history.seriesCount(), history.totalPoints());
}

void loadLocationHistoryFromJson(const string& filePath, SystemStatus* status) {
    ifstream input(filePath);
    if (!input.is_open()) return;
    size_t loaded = 0;
    string line;
    while (getline(input, line)) {
        if (line.empty() || line[0] != '{') continue;
        try {
            json j = json::parse(line);
            if (!j.contains("location")) continue;
            const auto& loc = j["location"];
            double lat = loc.value("latitude", 0.0);
            double lon = loc.value("longitude", 0.0);
            if (lat == 0.0 && lon == 0.0) continue;
            status->localLocationHistory.addPoint(lat, lon, loc.value("altitude", 0.0), loc.value("accuracy", 0.0f), loc.value("time", j.value("timestamp", 0LL)));
            ++loaded;
        } catch (const exception& e) {
            cout << "[JSON] Ошибка парсинга: " << e.what() << endl;
        }
    }
    cout << "[JSON] Загружено " << loaded << " точек GPS" << endl;
}

void loadSignalHistoryFromJson(const string& filePath, SystemStatus* status) {
    ifstream input(filePath);
    if (!input.is_open()) return;
    size_t loaded = 0;
    string line;
    while (getline(input, line)) {
        if (line.empty() || line[0] != '{') continue;
        try {
            json j = json::parse(line);
            if (!j.contains("telephony") || !j["telephony"].is_array()) continue;
            long long ts = j.value("timestamp", 0LL);
            if (ts == 0) continue;
            for (auto& item : j["telephony"]) {
                string type = item.value("type", "Unknown");
                string cellKey = type;
                int pci = -1, rsrp = -140, rssi = 0, sinr = 0;
                if (item.contains("CellIdentityLte")) {
                    auto cid = item["CellIdentityLte"];
                    pci = get_int_safe(cid, "PCI", -1);
                    if (pci != -1) cellKey += "_P" + to_string(pci);
                }
                if (item.contains("CellSignalStrengthLte")) {
                    auto ss = item["CellSignalStrengthLte"];
                    rsrp = get_int_safe(ss, "RSRP", -140); rssi = get_int_safe(ss, "RSSI", 0); sinr = get_int_safe(ss, "RSSNR", 0);
                } else {
                    rsrp = get_int_safe(item, "rsrp", -140);
                    if (rsrp == -140) rsrp = get_int_safe(item, "ssRsrp", -140);
                    rssi = get_int_safe(item, "rssi", 0); if (rssi == 0) rssi = get_int_safe(item, "dbm", 0);
                    sinr = get_int_safe(item, "sinr", 0);
                }
                if (rsrp != -140 || rssi != 0) {
                    status->localSignalHistory.addPoint(ts, rsrp, rssi, sinr, type, cellKey, pci);
                    ++loaded;
                }
            }
        } catch (const exception& e) {
            cout << "[JSON] Ошибка парсинга: " << e.what() << endl;
        }
    }
    cout << "[JSON] Загружено " << loaded << " точек сигнала" << endl;
}