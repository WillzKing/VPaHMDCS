#pragma once
#include "core/data_structures.h"
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>

namespace osm14 {

struct HeatmapPoint {
    double lat, lon, value;
};

enum class HeatmapCriterion { RSRP, RSRQ, RSSI, SINR, Altitude };

struct HeatmapConfig {
    HeatmapCriterion criterion{HeatmapCriterion::RSRP};
    float radius{30.0f};
    float power{2.0f};
    int zoom{17};           // zoom level to generate heatmap at
    bool perTile{false};    // future: generate per-tile heatmaps
};

struct HeatmapCache {
    unsigned int textureId{0};
    int pointCount{0};
    double minLon{0}, maxLon{0}, minLat{0}, maxLat{0};
    std::vector<unsigned char> pixels;
    int width{0}, height{0};
    int coloredPixels{0};
    std::string criterion;  // for debug logging
    std::string earfcn;     // for debug logging
};

HeatmapCache GenerateHeatmap(const std::vector<HeatmapPoint>& points, const HeatmapConfig& cfg, 
                              const std::string& criterion = "RSRP", const std::string& earfcn = "");
bool SaveHeatmapPNG(const HeatmapCache& cache, const std::string& filename);

class HeatmapGenerator {
public:
    HeatmapGenerator();
    ~HeatmapGenerator();

    void Start(const std::vector<HeatmapPoint>& points, const HeatmapConfig& cfg,
               const std::string& criterion_name = "RSRP", const std::string& earfcn = "");
    void Stop();
    bool IsReady() const { return ready_.load(); }
    bool GetResult(HeatmapCache& out);

private:
    std::thread worker_;
    std::atomic<bool> ready_{false};
    std::mutex mtx_;
    HeatmapCache cache_;
};

} // namespace osm14
