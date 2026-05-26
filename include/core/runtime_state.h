#pragma once
#include "data_structures.h"
#include "heatmap_generator.h"
#include <mutex>
#include <atomic>
#include <unordered_map>
#include <string>
#include <vector>
#include <functional>
#include <map>

struct RuntimeState {
    std::atomic<bool> running{true};
    std::mutex mtx;
    UserData currentUser;
    std::string lastRawJson;
    std::atomic<long long> packets{0};
    std::atomic<long long> bytesRx{0};

    std::vector<SignalData> signalSeries;
    std::unordered_map<std::string, size_t> seriesIndex;

    std::vector<MapPoint> heatmapPoints;

    std::function<void(const MobileNetworkInfo&)> onSignalStrengthChange;

    struct {
        int zoom = 15;
        double centerLat = 55.01, centerLon = 82.95;
        bool showHeat = false;
        float heatAlpha = 0.65f;
        
        bool heatTileMode = false;

        osm14::HeatmapCache currentHeat;
        osm14::HeatmapGenerator heatmapGen;
        osm14::HeatmapConfig heatmapConfig;
    } mapState;

    struct {
        std::mutex mtx;
        std::map<int, std::vector<double>> pciTime;
        std::map<int, std::vector<double>> pciRsrp;
        std::vector<double> trajLat, trajLon;
        long long baseTs = -1;
        bool loaded = false;
    } localData;
};