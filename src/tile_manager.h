#pragma once
#include "shared_structs.h"
#include <string>
#include <unordered_map>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
using namespace std;

class TileManager {
public:
    TileManager(const string& cacheDir = "cache");
    ~TileManager();
    
    void requestTile(int z, int x, int y);
    TileTexture* getTile(int z, int x, int y);
    void update();
    void clearQueue();
    void stop();
    void clearCache();
    size_t getCacheSize() const;
    
private:
    string makeKey(int z, int x, int y) const;
    string getTilePath(int z, int x, int y) const;
    void workerThreadFunc();
    
private:
    string cacheDir_;
    unordered_map<string, TileTexture> textures_;
    queue<string> taskQueue_;
    mutable mutex cacheMutex_;
    mutable mutex queueMutex_;
    condition_variable queueCV_;
    thread workerThread_;
    atomic<bool> running_{true};
};