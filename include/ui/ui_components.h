#pragma once
#include <mutex>
#include "core/runtime_state.h"

struct UIState {
    bool showDataWindow{true};
    bool showGraphWindow{true};
    bool showMapWindow{true};
    std::string selectedCellIdentity;
};

void DrawMenuBar(UIState& state);
void DrawDataWindow(bool& open, UserData& user, std::mutex& mtx);
void DrawGraphWindow(bool& open, std::vector<SignalData>& signals, std::string& selectedIdentity, std::mutex& mtx);
void DrawMapWindow(bool& open, RuntimeState& st);
