#include "ui/ui_components.h"
#include "osm_math.h"
#include "network/osm_tile_fetcher.h"
#include "network/osm_tile_texture.h"
#include "logging.h"
#include <imgui.h>
#include <implot.h>
#include <GL/glew.h>
#include <map>
#include <algorithm>
#include <mutex>
#include <vector>
#include <memory>
#include <filesystem>
#include "stb_image_write.h"
void DrawMapWindow(bool& open, RuntimeState& st) {
if (!open) return;
if (!ImGui::Begin("Карта View", &open)) { ImGui::End(); return; }
auto& ms = st.mapState;
static OsmTileFetcher fetcher(4);
static std::map<std::string, std::shared_ptr<OsmTileTexture>> textures;
static std::mutex texMtx;
if (ms.heatmapGen.IsReady()) {
osm14::HeatmapCache res;
if (ms.heatmapGen.GetResult(res)) {
if (ms.currentHeat.textureId) {
glDeleteTextures(1, &ms.currentHeat.textureId);
ms.currentHeat.textureId = 0;
}
ms.currentHeat.pointCount = res.pointCount;
ms.currentHeat.minLon = res.minLon;
ms.currentHeat.maxLon = res.maxLon;
ms.currentHeat.minLat = res.minLat;
ms.currentHeat.maxLat = res.maxLat;
ms.currentHeat.width = res.width;
ms.currentHeat.height = res.height;
ms.currentHeat.pixels = res.pixels;
ms.currentHeat.coloredPixels = res.coloredPixels;
ms.currentHeat.criterion = res.criterion;
ms.currentHeat.earfcn = res.earfcn;
glGenTextures(1, &ms.currentHeat.textureId);
glBindTexture(GL_TEXTURE_2D, ms.currentHeat.textureId);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
if (!ms.currentHeat.pixels.empty()) {
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
ms.currentHeat.width, ms.currentHeat.height,
0, GL_RGBA, GL_UNSIGNED_BYTE,
ms.currentHeat.pixels.data());
LogInfo("[MAP] Heatmap texture loaded: " +
std::to_string(ms.currentHeat.width) + "x" +
std::to_string(ms.currentHeat.height));
}
}
}
int z = ms.zoom;
double centerTileX = osm14::TileXExact(ms.centerLon, z);
double centerTileY = osm14::TileYExact(ms.centerLat, z);
ImVec2 plotSize = ImGui::GetContentRegionAvail();
double spanTileX = plotSize.x / (2.0 * 256.0);
double spanTileY = plotSize.y / (2.0 * 256.0);
double minX = centerTileX - spanTileX;
double maxX = centerTileX + spanTileX;
double minY = centerTileY - spanTileY;
double maxY = centerTileY + spanTileY;
ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
if (ImPlot::BeginPlot("##osm_map", plotSize, ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoLegend | ImPlotFlags_NoMouseText)) {
ImPlot::SetupAxes(nullptr, nullptr,
ImPlotAxisFlags_NoDecorations,
ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_Invert);
ImPlot::SetupAxisLimits(ImAxis_X1, minX, maxX, ImPlotCond_Always);
ImPlot::SetupAxisLimits(ImAxis_Y1, minY, maxY, ImPlotCond_Always);
int minTX = (int)std::floor(minX);
int maxTX = (int)std::ceil(maxX);
int minTY = (int)std::floor(minY);
int maxTY = (int)std::ceil(maxY);
std::vector<OsmTileCoord> toFetch;
for (int tx = minTX; tx <= maxTX; ++tx) {
for (int ty = minTY; ty <= maxTY; ++ty) {
std::string key = std::to_string(z) + ":" + std::to_string(tx) + ":" + std::to_string(ty);
std::shared_ptr<OsmTileTexture> tex;
{
std::lock_guard<std::mutex> lk(texMtx);
auto it = textures.find(key);
if (it == textures.end()) {
tex = std::make_shared<OsmTileTexture>();
textures[key] = tex;
toFetch.push_back({z, tx, ty});
} else {
tex = it->second;
}
}
if (tex) {
tex->GlLoad();
GLuint id = tex->GetId();
if (id) {
ImPlot::PlotImage(key.c_str(), (ImTextureID)(intptr_t)id,
ImPlotPoint(tx, ty), ImPlotPoint(tx + 1, ty + 1));
}
}
}
}
if (!toFetch.empty()) {
fetcher.Fetch(toFetch, [&](const OsmTileCoord& c, const std::vector<std::byte>& b) {
std::lock_guard<std::mutex> lk(texMtx);
std::string k = std::to_string(c.zoom) + ":" + std::to_string(c.x) + ":" + std::to_string(c.y);
auto it = textures.find(k);
if (it != textures.end()) {
it->second->StbLoad(b);
LogInfo("Tile loaded: " + k);
}
});
}
if (ms.showHeat && ms.currentHeat.textureId && !ms.currentHeat.pixels.empty()) {
bool isTileMode = ms.heatTileMode;
if (isTileMode) {
ImGui::SetNextWindowPos(ImVec2(10, 10));
ImGui::SetNextWindowBgAlpha(0.7f);
if (ImGui::Begin("Heatmap Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
ImGui::TextColored(ImVec4(1,1,0,1), "Режим по тайлам: генерация на тайл пока не реализована.");
ImGui::Text("Global map: %dx%d px", ms.currentHeat.width, ms.currentHeat.height);
ImGui::End();
}
} else {
double hMinX = osm14::TileXExact(ms.currentHeat.minLon, z);
double hMaxX = osm14::TileXExact(ms.currentHeat.maxLon, z);
double hMinY = osm14::TileYExact(ms.currentHeat.minLat, z);
double hMaxY = osm14::TileYExact(ms.currentHeat.maxLat, z);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
ImPlot::PlotImage("##heat", (ImTextureID)(intptr_t)ms.currentHeat.textureId,
ImPlotPoint(hMinX, hMinY), ImPlotPoint(hMaxX, hMaxY),
ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, ms.heatAlpha));
ImGui::SetNextWindowPos(ImVec2(10, 10));
ImGui::SetNextWindowBgAlpha(0.7f);
if (ImGui::Begin("Heatmap Info", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
ImGui::Text("Heatmap: %dx%d px", ms.currentHeat.width, ms.currentHeat.height);
ImGui::Text("Points: %d | Colored: %d", ms.currentHeat.pointCount, ms.currentHeat.coloredPixels);
ImGui::Text("Criterion: %s", ms.currentHeat.criterion.c_str());
if (!ms.currentHeat.earfcn.empty()) {
ImGui::Text("EARFCN: %s", ms.currentHeat.earfcn.c_str());
}
ImGui::End();
}
}
}
ImPlot::PlotScatter("##center", &centerTileX, &centerTileY, 1);
if (ImPlot::IsPlotHovered() && ImGui::IsMouseDragging(0)) {
ImVec2 md = ImGui::GetIO().MouseDelta;
centerTileX -= md.x / 256.0;
centerTileY += md.y / 256.0;
ms.centerLon = osm14::TileXToLon(centerTileX, z);
ms.centerLat = osm14::TileYToLat(centerTileY, z);
}
if (ImPlot::IsPlotHovered() && ImGui::GetIO().MouseWheel != 0) {
int dz = (ImGui::GetIO().MouseWheel > 0) ? 1 : -1;
int newZoom = std::clamp(z + dz, 3, 19);
if (newZoom != z) {
centerTileX = osm14::TileXExact(ms.centerLon, newZoom);
centerTileY = osm14::TileYExact(ms.centerLat, newZoom);
ms.zoom = newZoom;
z = newZoom;
}
}
ImPlot::EndPlot();
}
ImPlot::PopStyleVar();
ImGui::End();
}
bool SaveHeatmapPerTile(const osm14::HeatmapCache& cache, int zoom, const std::string& base_path) {
if (cache.pixels.empty() || cache.width <= 0) return false;
try {
int min_tx = (int)std::floor(osm14::TileXExact(cache.minLon, zoom));
int max_tx = (int)std::ceil(osm14::TileXExact(cache.maxLon, zoom));
int min_ty = (int)std::floor(osm14::TileYExact(cache.minLat, zoom));
int max_ty = (int)std::ceil(osm14::TileYExact(cache.maxLat, zoom));
std::string path = base_path + "/heatmap_" + cache.criterion +
(cache.earfcn.empty() ? "" : "_EARFCN" + cache.earfcn) + ".png";
return stbi_write_png(path.c_str(), cache.width, cache.height, 4,
cache.pixels.data(), cache.width * 4) != 0;
} catch (...) {
return false;
}
}