#pragma once
#include <cmath>

namespace osm14 {

const double PI = 3.14159265358979323846;

inline double MercatorX(double lon) { return lon; }

inline double MercatorY(double lat) {
    double latRad = lat * PI / 180.0;
    return std::log(std::tan(latRad) + 1.0 / std::cos(latRad));
}

inline double NormX(double mercX) { return 0.5 + mercX / 360.0; }

inline double NormY(double mercY) { return 0.5 - mercY / (2.0 * PI); }

inline int NumTiles(int zoom) { return 1 << zoom; }

inline double TileXExact(double lon, int zoom) {
    return (double)NumTiles(zoom) * NormX(MercatorX(lon));
}

inline double TileYExact(double lat, int zoom) {
    return (double)NumTiles(zoom) * NormY(MercatorY(lat));
}

inline double TileXToLon(double tx, int zoom) {
    return (tx / (double)NumTiles(zoom) - 0.5) * 360.0;
}

inline double TileYToLat(double ty, int zoom) {
    double mercY = (0.5 - ty / (double)NumTiles(zoom)) * (2.0 * PI);
    return std::atan(std::sinh(mercY)) * 180.0 / PI;
}

inline double InvMercatorY(double mercY) {
    return std::atan(std::sinh(mercY)) * 180.0 / PI;
}

} // namespace osm14
