#include "database.h"
#include <iostream>

Database::Database(const DBConfig& cfg) : config_(cfg) {
    reconnect();
    if (connected_) createTables();
}

Database::~Database() {
    if (conn_ && conn_->is_open()) conn_->close();
}

bool Database::reconnect() {
    try {
        std::string connStr = "host=" + config_.host + " port=" + config_.port +
                             " dbname=" + config_.dbname + " user=" + config_.user +
                             " password=" + config_.password;
        conn_ = std::make_unique<pqxx::connection>(connStr);
        if (conn_->is_open()) { connected_ = true; return true; }
    } catch (const std::exception& e) { 
        connected_ = false; 
        std::cerr << "[DB] " << e.what() << std::endl; 
    }
    return false;
}

bool Database::isConnected() {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!connected_ || !conn_ || !conn_->is_open()) return reconnect();
    return true;
}

void Database::createTables() {
    try {
        pqxx::work txn(*conn_);
        txn.exec(R"(CREATE TABLE IF NOT EXISTS location_history (
            id SERIAL PRIMARY KEY, latitude DOUBLE PRECISION NOT NULL,
            longitude DOUBLE PRECISION NOT NULL, altitude DOUBLE PRECISION DEFAULT 0,
            accuracy REAL DEFAULT 0, timestamp BIGINT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP))");
        txn.exec(R"(CREATE TABLE IF NOT EXISTS telephony_history (
            id SERIAL PRIMARY KEY, cell_type VARCHAR(50) NOT NULL,
            cell_key VARCHAR(100), mcc VARCHAR(10), mnc VARCHAR(10),
            cell_id VARCHAR(50), lac_tac VARCHAR(50), pci INTEGER DEFAULT -1,
            rsrp INTEGER DEFAULT -140, rssi INTEGER DEFAULT 0, sinr INTEGER DEFAULT 0,
            timestamp BIGINT NOT NULL, created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP))");
        txn.exec(R"(CREATE INDEX IF NOT EXISTS idx_location_ts ON location_history(timestamp);
            CREATE INDEX IF NOT EXISTS idx_telephony_ts ON telephony_history(timestamp))");
        txn.commit();
    } catch (const std::exception& e) { 
        std::cerr << "[DB] " << e.what() << std::endl; 
    }
}

bool Database::insertLocation(double lat, double lon, double alt, float acc, long long ts) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!connected_ || !conn_ || !conn_->is_open()) if (!reconnect()) return false;
    try {
        pqxx::work txn(*conn_);
        txn.exec_params("INSERT INTO location_history (latitude, longitude, altitude, accuracy, timestamp) VALUES ($1, $2, $3, $4, $5)",
            lat, lon, alt, acc, ts);
        txn.commit();
        return true;
    } catch (...) { return false; }
}

bool Database::insertTelephony(const CellInfo& cell, long long ts) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!connected_ || !conn_ || !conn_->is_open()) if (!reconnect()) return false;
    try {
        pqxx::work txn(*conn_);
        std::string cellKey = cell.getCellKey();
        std::string lacTac = cell.tac.empty() ? cell.lac : cell.tac;
        txn.exec_params("INSERT INTO telephony_history (cell_type, cell_key, mcc, mnc, cell_id, lac_tac, pci, rsrp, rssi, sinr, timestamp) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11)",
            cell.type, cellKey, cell.mcc, cell.mnc, cell.cellId, lacTac, 
            cell.pci, cell.signalStrength, cell.rssi, cell.sinr, ts);
        txn.commit();
        return true;
    } catch (...) { return false; }
}

int Database::insertNetworkUsage(long long sent, long long recv, long long ts, 
                                 const std::vector<AppUsage>& apps) {
    std::lock_guard<std::mutex> lock(dbMutex_);
    if (!connected_ || !conn_ || !conn_->is_open()) if (!reconnect()) return -1;
    try {
        pqxx::work txn(*conn_);
        pqxx::result res = txn.exec_params(
            "INSERT INTO network_usage_history (total_bytes_sent, total_bytes_received, timestamp) VALUES ($1, $2, $3) RETURNING id",
            sent, recv, ts);
        int netId = res[0][0].as<int>();
        for (const auto& app : apps) {
            txn.exec_params("INSERT INTO app_usage_history (package_name, bytes, network_usage_id, timestamp) VALUES ($1, $2, $3, $4)",
                app.packageName, app.bytes, netId, ts);
        }
        txn.commit();
        return netId;
    } catch (...) { return -1; }
}

DatabaseQueue::DatabaseQueue(Database* db) : db_(db) {
    worker_ = std::thread(&DatabaseQueue::workerThread, this);
}

DatabaseQueue::~DatabaseQueue() {
    running_.store(false);
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
}

void DatabaseQueue::workerThread() {
    while (running_.load()) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), [this] { 
                return !taskQueue_.empty() || !running_.load(); 
            });
            if (!taskQueue_.empty()) { 
                task = std::move(taskQueue_.front()); 
                taskQueue_.pop(); 
            }
        }
        if (task) task();
    }
}

void DatabaseQueue::enqueue(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mtx_);
    if (taskQueue_.size() < 5000) taskQueue_.push(std::move(task));
    cv_.notify_one();
}

size_t DatabaseQueue::pendingTasks() { 
    std::lock_guard<std::mutex> lock(mtx_); 
    return taskQueue_.size(); 
}