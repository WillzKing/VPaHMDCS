#pragma once
#include <string>
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <algorithm>
#include <GL/glew.h>
using namespace std;

struct DBConfig {
    string host = "localhost";
    string port = "5432";
    string dbname = "backend_control";
    string user = "postgres";
    string password = "2002";
};

struct AppUsage {
    string packageName;
    long long bytes;
};

struct CellInfo {
    string type = "Unknown";
    bool isRegistered = false;
    string mcc, mnc, cellId, lac, tac, nci;
    int pci = -1, earfcn = -1, nrarfcn = -1, arfcn = -1;
    int band = -1, bsic = -1, psc = -1;
    int rsrp = -140, rssi = 0, rsrq = 0, sinr = 0;
    int ssRsrp = -140, ssRsrq = 0, ssSinr = 0;
    int dbm = -140, rssnr = 0, asuLevel = 0, cqi = 0, timingAdvance = 0;
    int signalStrength = -140;
    
    string getCellKey() const {
        if (cellId.empty() || cellId == " " || cellId == "2147483647" || cellId == "0") {
            if (pci != -1) return mcc + "_" + mnc + "_PCI" + to_string(pci);
            if (!nci.empty() && nci != " ") return mcc + "_" + mnc + "_NCI" + nci;
        }
        string key = mcc + "_" + mnc + "_" + cellId;
        if (!tac.empty() && tac != " " && tac != "2147483647") key += "_" + tac;
        else if (!lac.empty() && lac != " " && lac != "2147483647") key += "_" + lac;
        return key;
    }
};

struct SignalPoint {
    long long timestamp;
    int rsrp = -140, rssi = 0, sinr = 0;
    string cellType, cellKey;
    int pci = -1;
};

struct LocationPoint {
    double latitude, longitude, altitude;
    float accuracy;
    long long timestamp;
};

struct TileTexture {
    GLuint textureId = 0;
    int width = 256, height = 256;
    bool valid = false;
    bool isLoading = false;
    vector<uint8_t> rgbaBlob;
};

struct MapState {
    double centerLat = 55.0084;
    double centerLon = 82.9357;
    int zoom = 15;
};

struct PciSignalHistory {
    unordered_map<int, vector<SignalPoint>> pciData;
    vector<SignalPoint> unknownPciData;
    mutable mutex mtx;
    long long baseTimestamp = 0;
    bool baseTimestampSet = false;
    static constexpr size_t MAX_POINTS = 2000;
    
    void addPoint(int pci, long long ts, int rsrp, int rssi, int sinr, 
                  const string& type, const string& cellKey = " ") {
        if (rsrp <= -140 && rssi == 0 && sinr == 0) return;
        lock_guard<mutex> lock(mtx);
        if (!baseTimestampSet) { baseTimestamp = ts; baseTimestampSet = true; }
        SignalPoint point{ts, rsrp, rssi, sinr, type, cellKey, pci};
        if (pci >= 0) {
            auto& vec = pciData[pci];
            vec.push_back(point);
            if (vec.size() > MAX_POINTS) vec.erase(vec.begin(), vec.begin() + (vec.size() - MAX_POINTS));
        } else {
            unknownPciData.push_back(point);
            if (unknownPciData.size() > MAX_POINTS) unknownPciData.erase(unknownPciData.begin(), unknownPciData.begin() + (unknownPciData.size() - MAX_POINTS));
        }
    }
    
    vector<int> getUniquePCIs() const {
        lock_guard<mutex> lock(mtx);
        vector<int> pcis;
        pcis.reserve(pciData.size());
        for (const auto& kv : pciData) pcis.push_back(kv.first);
        std::sort(pcis.begin(), pcis.end());
        return pcis;
    }
    
    pair<vector<double>, vector<double>> getGraphData(int pci, const function<double(const SignalPoint&)>& getValue) const {
        lock_guard<mutex> lock(mtx);
        vector<double> x, y;
        auto it = pciData.find(pci);
        if (it != pciData.end() && !it->second.empty()) {
            const auto& points = it->second;
            x.reserve(points.size()); y.reserve(points.size());
            for (const auto& pt : points) {
                x.push_back(static_cast<double>(pt.timestamp - baseTimestamp) / 1000.0);
                y.push_back(getValue(pt));
            }
        }
        return {move(x), move(y)};
    }
    
    size_t getTotalPoints() const {
        lock_guard<mutex> lock(mtx);
        size_t total = 0;
        for (const auto& kv : pciData) total += kv.second.size();
        total += unknownPciData.size();
        return total;
    }
    
    void clear() { lock_guard<mutex> lock(mtx); pciData.clear(); unknownPciData.clear(); baseTimestampSet = false; }
};

struct SignalHistory {
    unordered_map<string, vector<SignalPoint>> perCell;
    mutable mutex mtx;
    long long base_timestamp = 0;
    bool baseTimestampSet = false;
    
    void addPoint(long long ts, int rsrp, int rssi, int sinr, const string& type, 
                  const string& cellKey = " ", int pci = -1) {
        lock_guard<mutex> lock(mtx);
        if (rsrp <= -140 && rssi == 0) return;
        string key = cellKey.empty() ? type : cellKey;
        if (!baseTimestampSet) { base_timestamp = ts; baseTimestampSet = true; }
        auto& vec = perCell[key];
        vec.push_back({ts, rsrp, rssi, sinr, type, key, pci});
        if (vec.size() > 50000) vec.erase(vec.begin());
    }
    
    unordered_map<string, pair<vector<double>, vector<double>>> getPlotData(const function<double(const SignalPoint&)>& getValue) const {
        lock_guard<mutex> lock(mtx);
        unordered_map<string, pair<vector<double>, vector<double>>> result;
        for (const auto& kv : perCell) {
            vector<double> x, y;
            for (const auto& pt : kv.second) {
                x.push_back(static_cast<double>(pt.timestamp - base_timestamp) / 1000.0);
                y.push_back(getValue(pt));
            }
            if (!x.empty()) result[kv.first] = {move(x), move(y)};
        }
        return result; 
    }
    
    size_t totalPoints() const { lock_guard<mutex> lock(mtx); size_t t = 0; for (const auto& kv : perCell) t += kv.second.size(); return t; }
    size_t seriesCount() const { lock_guard<mutex> lock(mtx); return perCell.size(); }
    void reset() { lock_guard<mutex> lock(mtx); perCell.clear(); baseTimestampSet = false; base_timestamp = 0; }
};

struct LocationHistory {
    vector<LocationPoint> points;
    static constexpr size_t MAX_POINTS = 50000;
    mutable mutex mtx;
    
    void addPoint(double lat, double lon, double alt, float acc, long long ts) {
        lock_guard<mutex> lock(mtx);
        if (lat == 0.0 && lon == 0.0) return;
        points.push_back({lat, lon, alt, acc, ts});
        if (points.size() > MAX_POINTS) points.erase(points.begin(), points.begin() + (points.size() - MAX_POINTS));
    }
    
    pair<vector<double>, vector<double>> getPlotData() const {
        lock_guard<mutex> lock(mtx);
        vector<double> lats, lons;
        for (const auto& p : points) { lats.push_back(p.latitude); lons.push_back(p.longitude); }
        return {lats, lons};
    }
    
    void clear() { lock_guard<mutex> lock(mtx); points.clear(); }
};

struct SystemStatus {
    mutex mtx;
    atomic<long long> lastMessageTime{0};
    atomic<int> messageCount{0}, messagesPerSecond{0};
    double lat = 0, lon = 0, alt = 0;
    float accuracy = 0;
    long long timestamp = 0;
    long long netSent = 0, netRecv = 0;
    vector<AppUsage> topApps;
    vector<CellInfo> cells;
    bool filterLocation = true, filterTelephony = true, filterNetwork = true;
    bool has_connection = false;
    string telephonySummary = "Нет данных";
    MapState map;
    atomic<int> dbInsertCount{0};
    atomic<size_t> dbPendingTasks{0};
    SignalHistory signalHistory;
    SignalHistory localSignalHistory;
    LocationHistory locationHistory;
    LocationHistory localLocationHistory;
    PciSignalHistory pciSignalHistory;
};