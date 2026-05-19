#pragma once
#include <atomic>
#include "shared_structs.h"
#include "database.h"
#include "tile_manager.h"
#include "map_renderer.h"
using namespace std;
void run_gui_thread(SystemStatus* shared_data, Database* db, DatabaseQueue* dbQueue, atomic<bool>* should_stop);