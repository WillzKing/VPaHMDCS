#pragma once
#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <implot.h>
#include "shared_structs.h"
using namespace std;
using json = nlohmann::json;

string formatTimestamp(long long ts);
string formatBytes(long long bytes);
string formatRSRP(int rsrp);
ImVec4 getRSRPColor(int rsrp);
string get_string_safe(const json& val);
string get_string_safe(const json& obj, const string& key);
int get_int_safe(const json& obj, const string& key, int defaultVal = -140);
void renderCellDetails(const CellInfo& c);
ImVec4 getPciColor(int pci);
void renderPciSignalGraph(PciSignalHistory& history, const string& title,
                          const function<double(const SignalPoint&)>& getValue,
                          const string& yAxisLabel, double yMin, double yMax);
void renderSignalGraph(SignalHistory& history, const string& title,
                       const function<double(const SignalPoint&)>& getValue,
                       const string& yAxisLabel, double yMin, double yMax);
void loadLocationHistoryFromJson(const string& filePath, SystemStatus* status);
void loadSignalHistoryFromJson(const string& filePath, SystemStatus* status);