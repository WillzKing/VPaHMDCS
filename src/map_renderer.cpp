#include "map_renderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
using namespace std;

MapRenderer::MapRenderer(TileManager& tileManager) : tileManager_(tileManager) {}
MapRenderer::~MapRenderer() {}

double MapRenderer::latToMercatorImPlot(double lat) {
    double latRad = lat * M_PI / 180.0;
    return asinh(tan(latRad)) * 180.0 / M_PI;
}

static double lonToTileX(double lon, int zoom) { 
    return (0.5 + lon / 360.0) * (1 << zoom); 
}

static double mercYToTileY(double mercY, int zoom) { 
    return (0.5 - mercY / 360.0) * (1 << zoom); 
}

static double tileXToLon(double x, int zoom) {           // ← было int → теперь double
    return (x / (double)(1 << zoom) - 0.5) * 360.0; 
}

static double tileYToMercY(double y, int zoom) {         // ← было int → теперь double
    return (0.5 - y / (double)(1 << zoom)) * 360.0; 
}

void MapRenderer::handleInput(MapState& map, const ImVec2& canvasPos, const ImVec2& canvasSize) {
    ImGuiIO& io = ImGui::GetIO();
    bool hovered = io.MousePos.x >= canvasPos.x && io.MousePos.y >= canvasPos.y &&
                   io.MousePos.x <= canvasPos.x + canvasSize.x && io.MousePos.y <= canvasPos.y + canvasSize.y;
    
    if (!hovered) { 
        isDragging_ = false; 
        return; 
    }

    // ==================== ЗУМ КОЛЕСИКОМ (с сохранением позиции под курсором) ====================
    if (io.MouseWheel != 0.0f) {
        int oldZoom = map.zoom;
        map.zoom += (io.MouseWheel > 0.0f) ? 1 : -1;
        map.zoom = std::clamp(map.zoom, 1, 19);

        if (map.zoom != oldZoom) {
            // Позиция под курсором остаётся на месте
            double oldTileX = lonToTileX(map.centerLon, oldZoom);
            double oldMercY = latToMercatorImPlot(map.centerLat);
            double oldTileY = mercYToTileY(oldMercY, oldZoom);

            double relX = (io.MousePos.x - canvasPos.x) - canvasSize.x * 0.5;
            double relY = (io.MousePos.y - canvasPos.y) - canvasSize.y * 0.5;

            double mouseTileX = oldTileX + relX / 256.0;
            double mouseTileY = oldTileY + relY / 256.0;

            double mouseLon = tileXToLon(mouseTileX, oldZoom);           // <-- double, без (int)
            double mouseMercY = tileYToMercY(mouseTileY, oldZoom);
            double mouseLat = atan(sinh(mouseMercY * M_PI / 180.0)) * 180.0 / M_PI;

            double newTileX = lonToTileX(mouseLon, map.zoom);
            double newMercY = latToMercatorImPlot(mouseLat);
            double newTileY = mercYToTileY(newMercY, map.zoom);

            double newCenterTileX = newTileX - relX / 256.0;
            double newCenterTileY = newTileY - relY / 256.0;

            map.centerLon = tileXToLon(newCenterTileX, map.zoom);        // <-- double!
            map.centerLat = atan(sinh(tileYToMercY(newCenterTileY, map.zoom) * M_PI / 180.0)) * 180.0 / M_PI;

            tileManager_.clearQueue();
        }
    }

    // ==================== ПЕРЕТАСКИВАНИЕ МЫШКОЙ ====================
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        isDragging_ = true;
        lastMousePos_ = io.MousePos;
    }

    if (isDragging_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        ImVec2 delta(io.MousePos.x - lastMousePos_.x, io.MousePos.y - lastMousePos_.y);

        double centerTileX = lonToTileX(map.centerLon, map.zoom);
        double centerMercY = latToMercatorImPlot(map.centerLat);
        double centerTileY = mercYToTileY(centerMercY, map.zoom);

        // Основное исправление: убрали (int) + теперь всё double
        centerTileX -= delta.x / 256.0;
        centerTileY -= delta.y / 256.0;   // направление Y уже правильное

        map.centerLon = tileXToLon(centerTileX, map.zoom);           // <-- без (int)!
        map.centerLat = atan(sinh(tileYToMercY(centerTileY, map.zoom) * M_PI / 180.0)) * 180.0 / M_PI;

        lastMousePos_ = io.MousePos;
    }

    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        isDragging_ = false;
    }
}

void MapRenderer::render(MapState& map, const SystemStatus& status, const ImVec2& canvasSize) {
    tileManager_.update();

    if (ImPlot::BeginPlot("##OSM", ImVec2(-1, -1), ImPlotFlags_CanvasOnly)) {
        
        // === НОВОЕ: Синхронизируем лимиты ImPlot с центром и зумом ===
        double zoomFactor = 1 << map.zoom;
        double lonPerTile  = 360.0 / zoomFactor;
        double mercPerTile = 360.0 / zoomFactor;

        double tilesWideHalf  = (canvasSize.x / 256.0) / 2.0;
        double tilesHighHalf  = (canvasSize.y / 256.0) / 2.0;

        double leftLon  = map.centerLon - tilesWideHalf * lonPerTile;
        double rightLon = map.centerLon + tilesWideHalf * lonPerTile;

        double centerMercY = latToMercatorImPlot(map.centerLat);
        double topMercY    = centerMercY + tilesHighHalf * mercPerTile;
        double bottomMercY = centerMercY - tilesHighHalf * mercPerTile;

        ImPlot::SetupAxisLimits(ImAxis_X1, leftLon,  rightLon,  ImPlotCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, bottomMercY, topMercY, ImPlotCond_Always);
        // ========================================================

        ImPlotRect limits = ImPlot::GetPlotLimits();

        // Расчет границ тайлов (остальной код без изменений)
        int minX = (int)floor(lonToTileX(limits.X.Min, map.zoom));
        int maxX = (int)floor(lonToTileX(limits.X.Max, map.zoom));

        int minY = (int)floor(mercYToTileY(limits.Y.Max, map.zoom));
        int maxY = (int)floor(mercYToTileY(limits.Y.Min, map.zoom));

        int maxIdx = (1 << map.zoom) - 1;
        minX = clamp(minX, 0, maxIdx); maxX = clamp(maxX, 0, maxIdx);
        minY = clamp(minY, 0, maxIdx); maxY = clamp(maxY, 0, maxIdx);

        int totalTiles = (maxX - minX + 1) * (maxY - minY + 1);
        if (totalTiles > 100) {
            ImPlot::EndPlot();
            return;
        }

        for (int x = minX; x <= maxX; ++x) {
            for (int y = minY; y <= maxY; ++y) {
                TileTexture* tex = tileManager_.getTile(map.zoom, x, y);
                ImPlotPoint pMin = { tileXToLon(x, map.zoom), tileYToMercY(y + 1, map.zoom) };
                ImPlotPoint pMax = { tileXToLon(x + 1, map.zoom), tileYToMercY(y, map.zoom) };

                if (tex) { 
                    ImPlot::PlotImage("##tile", (void*)(intptr_t)tex->textureId, pMin, pMax); 
                } else { 
                    tileManager_.requestTile(map.zoom, x, y); 
                }
            }
        }

        // Маркер GPS
        if (status.lat != 0.0 || status.lon != 0.0) {
            double gpsX = status.lon;
            double gpsY = latToMercatorImPlot(status.lat);
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1, 0, 0, 1));
            ImPlot::PlotScatter("GPS", &gpsX, &gpsY, 1);
            ImPlot::PopStyleColor();
        }
        
        ImPlot::EndPlot();
    }
}