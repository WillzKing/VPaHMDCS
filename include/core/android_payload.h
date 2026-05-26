#pragma once
#include <string>
#include <vector>

struct CellPayload {
    std::string type; // LTE/NR/GSM
    bool isRegistered{false};
    int pci{-1};
    int earfcn{-1}, nrarfcn{-1}, arfcn{-1};
    int band{-1};
    std::string tac, lac, nci, cellId, mcc, mnc;
    int rsrp{-140}, rsrq{0}, rssi{0}, rssnr{0}, sinr{0}, cqi{0}, asuLevel{0}, timingAdvance{0};
};

struct LocationPayload {
    double latitude{0.0}, longitude{0.0}, altitude{0.0};
    long long time{0};
    float accuracy{0.0f};
};

struct CombinedPayload {
    std::string type{"combined"};
    long long timestamp{0};
    float device_orientation{0.0f};
    LocationPayload location;
    std::vector<CellPayload> telephony;
};
