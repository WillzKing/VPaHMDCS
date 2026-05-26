#include "core/heatmap_generator.h"
#include "osm_math.h"
#include "logging.h"
#include <cmath>
#include <algorithm>
#include <GL/glew.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
namespace osm14 {
struct Color { int r, g, b, a; };
double haversine(double lat1, double lon1, double lat2, double lon2) {
double R = 6371000.0;
double dLat = (lat2 - lat1) * M_PI / 180.0;
double dLon = (lon2 - lon1) * M_PI / 180.0;
double a = sin(dLat/2)*sin(dLat/2) +
cos(lat1*M_PI/180.0)*cos(lat2*M_PI/180.0) *
sin(dLon/2)*sin(dLon/2);
return R * 2 * atan2(sqrt(a), sqrt(1-a));
}
Color get_color(double val, HeatmapCriterion criterion) {
if (criterion == HeatmapCriterion::RSRP) {
if (val < -120) return {60, 60, 60, 180};
if (val <= -105) return {0, 0, 139, 255};
if (val <= -95)  return {0, 255, 255, 255};
if (val <= -85)  return {255, 165, 0, 255};
return {255, 0, 0, 255};
}
if (criterion == HeatmapCriterion::RSRQ) {
if (val < -24) return {60, 60, 60, 180};
if (val <= -18) return {0, 0, 139, 255};
if (val <= -12) return {0, 255, 255, 255};
if (val <= -8)  return {255, 165, 0, 255};
return {255, 0, 0, 255};
}
if (criterion == HeatmapCriterion::RSSI) {
if (val < -115) return {60, 60, 60, 180};
if (val <= -95) return {0, 0, 139, 255};
if (val <= -85) return {0, 255, 255, 255};
if (val <= -75) return {255, 165, 0, 255};
return {255, 0, 0, 255};
}
if (criterion == HeatmapCriterion::Altitude) {
if (val < 0) return {60, 60, 60, 180};
if (val > 500) val = 500;
uint8_t intensity = (uint8_t)(255 * (val / 500.0));
uint8_t blue = (uint8_t)(255 - intensity);
return {intensity, 0, blue, 255};
}
if (val < -10) return {60, 60, 60, 180};
if (val > 30) val = 30;
double norm = (val + 10) / 40.0;
uint8_t r = (uint8_t)(255 * norm);
uint8_t b = (uint8_t)(255 * (1.0 - norm));
return {r, (uint8_t)(255 * (1.0 - std::abs(2.0*norm - 1.0))), b, 255};
}
std::string criterion_to_string(HeatmapCriterion c) {
switch(c) {
case HeatmapCriterion::RSRP: return "RSRP";
case HeatmapCriterion::RSRQ: return "RSRQ";
case HeatmapCriterion::RSSI: return "RSSI";
case HeatmapCriterion::SINR: return "SINR";
case HeatmapCriterion::Altitude: return "Altitude";
default: return "Unknown";
}
}
bool SaveHeatmapPNG(const HeatmapCache& cache, const std::string& filename) {
if (cache.pixels.empty() || cache.width <= 0 || cache.height <= 0) {
LogError("[HEATMAP] Cannot save: empty pixels or invalid dimensions");
return false;
}
try {
std::filesystem::create_directories(std::filesystem::path(filename).parent_path());
int result = stbi_write_png(filename.c_str(), cache.width, cache.height, 4,
cache.pixels.data(), cache.width * 4);
if (result) {
LogInfo("[HEATMAP] Saved PNG: " + filename + " (" +
std::to_string(cache.width) + "x" + std::to_string(cache.height) + ")");
return true;
} else {
LogError("[HEATMAP] Failed to write PNG: " + filename);
return false;
}
} catch (const std::exception& e) {
LogError("[HEATMAP] Exception saving PNG: " + std::string(e.what()));
return false;
}
}
HeatmapCache GenerateHeatmap(const std::vector<HeatmapPoint>& pts,
const HeatmapConfig& cfg,
const std::string& criterion_name,
const std::string& earfcn) {
std::cout << "[HEATMAP] ===== STARTED =====" << std::endl;
std::cout << "[HEATMAP] Points count: " << pts.size() << std::endl;
std::cout << "[HEATMAP] Criterion: " << criterion_name << ", EARFCN: " << earfcn << std::endl;
HeatmapCache c{};
if (pts.empty()) {
std::cout << "[HEATMAP] ABORT: No data points" << std::endl;
return c;
}
double minLat = 90, maxLat = -90, minLon = 180, maxLon = -180;
for (const auto& p : pts) {
if (p.lat < minLat) minLat = p.lat;
if (p.lat > maxLat) maxLat = p.lat;
if (p.lon < minLon) minLon = p.lon;
if (p.lon > maxLon) maxLon = p.lon;
}
double pad = 0.001;
minLat -= pad; maxLat += pad;
minLon -= pad; maxLon += pad;
std::cout << "[HEATMAP] BBOX: lat[" << minLat << "," << maxLat
<< "] lon[" << minLon << "," << maxLon << "]" << std::endl;
const int W = 400, H = 400;
c.width = W;
c.height = H;
c.minLon = minLon;
c.maxLon = maxLon;
c.minLat = minLat;
c.maxLat = maxLat;
c.pointCount = (int)pts.size();
c.criterion = criterion_name;
c.earfcn = earfcn;
float effective_radius = std::clamp(cfg.radius, 10.0f, 40.0f);
std::cout << "[HEATMAP] Grid: " << W << "x" << H << ", radius_m: " << effective_radius << std::endl;
std::vector<double> sumW(W * H, 0.0);
std::vector<double> sumV(W * H, 0.0);
std::vector<unsigned char> pixels(W * H * 4, 0);
std::cout << "[HEATMAP] Starting IDW interpolation..." << std::endl;
for (const auto& pt : pts) {
double pt_dLat = (effective_radius / 111320.0);
double pt_dLon = (effective_radius / (111320.0 * std::cos(pt.lat * M_PI / 180.0)));
int x0 = (int)((pt.lon - pt_dLon - minLon) / (maxLon - minLon) * W) - 1;
int x1 = (int)((pt.lon + pt_dLon - minLon) / (maxLon - minLon) * W) + 1;
int y0 = (int)((maxLat - (pt.lat + pt_dLat)) / (maxLat - minLat) * H) - 1;
int y1 = (int)((maxLat - (pt.lat - pt_dLat)) / (maxLat - minLat) * H) + 1;
x0 = std::max(0, x0); x1 = std::min(W - 1, x1);
y0 = std::max(0, y0); y1 = std::min(H - 1, y1);
for (int py = y0; py <= y1; ++py) {
double p_lat = maxLat - (maxLat - minLat) * ((double)py / H);
for (int px = x0; px <= x1; ++px) {
double p_lon = minLon + (maxLon - minLon) * ((double)px / W);
double d = haversine(p_lat, p_lon, pt.lat, pt.lon);
if (d <= effective_radius) {
double dist = std::max(0.1, d);
double w = 1.0 / std::pow(dist, cfg.power);
sumW[py * W + px] += w;
sumV[py * W + px] += w * pt.value;
}
}
}
}
double min_z = 1e9, max_z = -1e9;
int colored_pixels = 0;
for (int i = 0; i < W * H; ++i) {
if (sumW[i] > 1e-12) {
double z = sumV[i] / sumW[i];
if (z < min_z) min_z = z;
if (z > max_z) max_z = z;
Color col = get_color(z, cfg.criterion);
pixels[i * 4 + 0] = col.r;
pixels[i * 4 + 1] = col.g;
pixels[i * 4 + 2] = col.b;
pixels[i * 4 + 3] = col.a;
colored_pixels++;
} else {
pixels[i * 4 + 0] = 0;
pixels[i * 4 + 1] = 0;
pixels[i * 4 + 2] = 0;
pixels[i * 4 + 3] = 0;
}
}
std::cout << "[HEATMAP] Value range: [" << min_z << ", " << max_z << "]" << std::endl;
std::cout << "[HEATMAP] DONE: colored=" << colored_pixels
<< " (" << (100.0 * colored_pixels / (W * H)) << "%)" << std::endl;
c.pixels = std::move(pixels);
c.coloredPixels = colored_pixels;
std::string base_dir = "build";
try {
std::filesystem::create_directories(base_dir);
} catch (...) {}
std::string big_path = base_dir + "/heatmap_" + criterion_name +
(earfcn.empty() ? "" : "_EARFCN" + earfcn) + ".png";
if (stbi_write_png(big_path.c_str(), W, H, 4, c.pixels.data(), W * 4)) {
std::cout << "[HEATMAP] Big image saved: " << big_path << std::endl;
auto fsize = std::filesystem::file_size(big_path);
std::cout << "[HEATMAP] File size: " << fsize << " bytes" << std::endl;
} else {
std::cerr << "[HEATMAP] Failed to save big image: " << big_path << std::endl;
}
std::cout << "[HEATMAP] ===== FINISHED =====" << std::endl;
return c;
}
HeatmapGenerator::HeatmapGenerator() {}
HeatmapGenerator::~HeatmapGenerator() { Stop(); }
void HeatmapGenerator::Stop() {
if (worker_.joinable()) {
worker_.join();
}
}
void HeatmapGenerator::Start(const std::vector<HeatmapPoint>& pts,
const HeatmapConfig& cfg,
const std::string& criterion_name,
const std::string& earfcn) {
Stop();
ready_ = false;
auto points_copy = pts;
auto cfg_copy = cfg;
auto crit_copy = criterion_name;
auto earfcn_copy = earfcn;
worker_ = std::thread([this, points_copy, cfg_copy, crit_copy, earfcn_copy]() {
try {
auto res = GenerateHeatmap(points_copy, cfg_copy, crit_copy, earfcn_copy);
{
std::lock_guard<std::mutex> lk(mtx_);
cache_ = res;
}
ready_ = true;
LogInfo("[HEATMAP] Result ready in cache");
} catch (const std::exception& e) {
LogError("[HEATMAP] Exception in worker: " + std::string(e.what()));
ready_ = false;
}
});
}
bool HeatmapGenerator::GetResult(HeatmapCache& out) {
if (!ready_.load()) return false;
std::lock_guard<std::mutex> lk(mtx_);
if (cache_.pixels.empty()) return false;
out = cache_;
ready_ = false;
return true;
}
} // namespace osm14