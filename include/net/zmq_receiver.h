#pragma once
#include "core/runtime_state.h"

struct FilterState {
    bool gps = true;
    bool lte = true;
    bool nr = true;
};

void RunZmqReceiver(RuntimeState* st, int dataPort = 5558, int cmdPort = 5559, bool writeDb = true);
FilterState& GetFilterState();
void SendFilterCommand(const std::string& serverIp, int cmdPort, const FilterState& filters);
