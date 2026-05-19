#pragma once
#include <atomic>
#include "shared_structs.h"
#include "database.h"

using namespace std;

void run_server_thread(SystemStatus* shared_data, Database* db, DatabaseQueue* dbQueue, atomic<bool>* should_stop);