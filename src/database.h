#pragma once
#include "shared_structs.h"
#include <pqxx/pqxx>
#include <memory>
#include <queue>
#include <functional>

class Database {
public:
    Database(const DBConfig& cfg);
    ~Database();
    
    bool isConnected();
    bool insertLocation(double lat, double lon, double alt, float acc, long long ts);
    bool insertTelephony(const CellInfo& cell, long long ts);
    int insertNetworkUsage(long long sent, long long recv, long long ts, 
                          const std::vector<AppUsage>& apps);
    
private:
    bool reconnect();
    void createTables();
    
    DBConfig config_;
    std::mutex dbMutex_;
    std::unique_ptr<pqxx::connection> conn_;
    bool connected_ = false;
};

class DatabaseQueue {
public:
    DatabaseQueue(Database* db);
    ~DatabaseQueue();
    
    void enqueue(std::function<void()> task);
    size_t pendingTasks();
    
private:
    void workerThread();
    
    Database* db_;
    std::queue<std::function<void()>> taskQueue_;
    std::mutex mtx_;
    std::condition_variable cv_;
    std::atomic<bool> running_{true};
    std::thread worker_;
};