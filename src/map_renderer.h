#pragma once
#include "shared_structs.h"
#include "tile_manager.h"
#include <imgui.h>
#include <implot.h>

using namespace std;

class MapRenderer {
public:
    MapRenderer(TileManager& tileManager);
    ~MapRenderer();
    void handleInput(MapState& map, const ImVec2& canvasPos, const ImVec2& canvasSize);
    void render(MapState& map, const SystemStatus& status, const ImVec2& canvasSize);
private:
    static double latToMercatorImPlot(double lat);
    TileManager& tileManager_;
    bool isDragging_ = false;
    ImVec2 lastMousePos_;
};