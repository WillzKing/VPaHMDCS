#include "logging.h"
#include <mutex>
#include <fstream>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <ctime>

static std::mutex _log_mtx;
static std::ofstream _log_file;

static std::ofstream& GetLogFile() {
    if (!_log_file.is_open()) {
        std::error_code ec; std::filesystem::create_directories("build", ec);
        _log_file.open("build\\app.log", std::ios::app);
    }
    return _log_file;
}

void LogInfo(const std::string& msg) {
    std::lock_guard<std::mutex> lk(_log_mtx);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{}; localtime_s(&tm, &t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [INFO] " << msg << "\n";
    std::cout << ss.str();
    auto& f = GetLogFile(); if (f.is_open()) f << ss.str();
}

void LogError(const std::string& msg) {
    std::lock_guard<std::mutex> lk(_log_mtx);
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{}; localtime_s(&tm, &t);
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << " [ERROR] " << msg << "\n";
    std::cerr << ss.str();
    auto& f = GetLogFile(); if (f.is_open()) f << ss.str();
}
