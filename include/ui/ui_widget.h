#pragma once
#include "core/data_structures.h"
#include <mutex>
#include <vector>

void CreateMobileNetworkWidget(UserData& currentUser, std::mutex& dataMutex, std::vector<SignalData>& signalData);
void SetupAmethystStyle();
