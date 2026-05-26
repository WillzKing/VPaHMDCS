#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

struct OsmTileCoord { int zoom{0}; int x{0}; int y{0}; };

class OsmTileFetcher {
public:
    explicit OsmTileFetcher(size_t threads = 2);
    ~OsmTileFetcher();
    void ClearJobs();
    void Fetch(const std::vector<OsmTileCoord>& coords, std::function<void(const OsmTileCoord&, const std::vector<std::byte>&)> callback);

private:
    struct Job { 
        OsmTileCoord coord; 
        std::function<void(const OsmTileCoord&, const std::vector<std::byte>&)> callback; 
    };
    std::vector<std::thread> workers;
    std::queue<Job> jobs;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stopPool{false};

    static size_t OnPullResponse(void* data, size_t size, size_t nmemb, void* userp);
    void ProcessJob(const Job& job);
    void WorkerLoop();
};