#pragma once
#include "core/runtime_state.h"

bool GuiAppInit(int width, int height, const char* title);

bool GuiAppFrame(RuntimeState& state);

void GuiAppShutdown();
