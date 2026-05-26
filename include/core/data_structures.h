#pragma once
#include <string>
#include <vector>

struct MobileNetworkInfo {
    std::string networkType;
    std::string cellIdentity;
    std::string mcc, mnc;
    int pci{0}, tac{0};
    int rsrp{-140}, rsrq{0}, rssi{0};
    int earfcn{-1}, nrarfcn{-1}, arfcn{-1};
    int timingAdvance{0};
    int cqi{0};
    double sinr{0.0};
    std::string band;
};

struct LocationInfo {
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
    float accuracy{0.0f};
    long long timestamp{0};
};

struct UserData {
    std::string user{"Android"};
    LocationInfo location;
    std::vector<LocationInfo> trajectory;
    std::vector<MobileNetworkInfo> mobileNetworks;
    long long totalBytesSent{0};
    long long totalBytesReceived{0};
    std::vector<std::string> topTrafficApps;
};

struct SignalData {
    std::string cellIdentity;
    std::vector<double> timestamps;
    std::vector<double> rsrp_values;
    std::vector<double> rsrq_values;
    std::vector<double> rssi_values;
};

struct MapPoint {
    double latitude{0.0};
    double longitude{0.0};
    double altitude{0.0};
    int rsrp{-140};
    int rsrq{0};
    int rssi{0};
    double sinr{0.0};
    long long time{0};
    int earfcn{-1};
    int pci{-1};
};