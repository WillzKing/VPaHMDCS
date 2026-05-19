#include <iostream>
#include <thread>
#include <atomic>
#include "curl_funcs.h"
#include "database.h"
#include "app_server.h"
#include "app_gui.h"
#include "app_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    cout << "=== Центр управления Backend ===\n";
    CurlFuncs curl;
    DBConfig dbCfg;
    Database db(dbCfg);
    DatabaseQueue dbQueue(&db);
    SystemStatus status;
    atomic<bool> stop{false};
    status.map.zoom = 15;
    if (!curl.init()) return 1;
    loadLocationHistoryFromJson("drive_test_log.json", &status);
    loadSignalHistoryFromJson("drive_test_log.json", &status);
    thread server(run_server_thread, &status, &db, &dbQueue, &stop);
    run_gui_thread(&status, &db, &dbQueue, &stop);
    stop.store(true);
    if (server.joinable()) server.join();
    curl.cleanup();
    return 0;
}