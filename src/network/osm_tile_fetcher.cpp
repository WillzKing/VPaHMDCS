#include "network/osm_tile_fetcher.h"
#include "logging.h"
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <sstream>

OsmTileFetcher::OsmTileFetcher(size_t threads) {
    for (size_t i = 0; i < threads; ++i) workers.emplace_back(&OsmTileFetcher::WorkerLoop, this);
}
OsmTileFetcher::~OsmTileFetcher() {
    { std::unique_lock<std::mutex> lock(queueMutex); stopPool = true; }
    condition.notify_all();
    for (std::thread& worker : workers) if (worker.joinable()) worker.join();
}
void OsmTileFetcher::ClearJobs() { std::unique_lock<std::mutex> lock(queueMutex); while (!jobs.empty()) jobs.pop(); }
size_t OsmTileFetcher::OnPullResponse(void* data, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto& blob = *static_cast<std::vector<std::byte>*>(userp);
    auto const* dataptr = static_cast<std::byte*>(data);
    blob.insert(blob.cend(), dataptr, dataptr + realsize);
    return realsize;
}
void OsmTileFetcher::ProcessJob(const Job& job) {
    std::vector<std::byte> blob;
    std::filesystem::path dir = std::filesystem::path("build") / std::to_string(job.coord.zoom) / std::to_string(job.coord.x);
    std::filesystem::path filePath = dir / (std::to_string(job.coord.y) + ".png");

    if (std::filesystem::exists(filePath)) {
        LogInfo("Tile found on disk: " + filePath.string());
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            auto size = file.tellg(); file.seekg(0, std::ios::beg); blob.resize((size_t)size);
            file.read(reinterpret_cast<char*>(blob.data()), size);
            LogInfo("Read tile bytes: " + std::to_string(blob.size()) + " for " + filePath.string());
            job.callback(job.coord, blob);
            return;
        } else {
            LogError("Failed to open tile file: " + filePath.string());
        }
    }
    CURL* curl = curl_easy_init(); if (!curl) { LogError("curl init failed"); return; }
    std::ostringstream urlmaker; urlmaker << "https://tile.openstreetmap.org/" << job.coord.zoom << '/' << job.coord.x << '/' << job.coord.y << ".png";
    std::string url = urlmaker.str();
    LogInfo("Fetching URL: " + url);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "rework_osm_step14/1.0");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&blob);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnPullResponse);
    CURLcode res = curl_easy_perform(curl); curl_easy_cleanup(curl);
    if (res == CURLE_OK && !blob.empty()) {
        constexpr size_t MIN_VALID_TILE_SIZE = 1024;
        if (blob.size() < MIN_VALID_TILE_SIZE) {
            LogError("[TILE_INVALID] Data too small: " + std::to_string(blob.size()) + " bytes (min " + std::to_string(MIN_VALID_TILE_SIZE) + ") for " + url);
            return;
        }
        
        std::error_code ec; std::filesystem::create_directories(dir, ec);
        std::ofstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            file.write(reinterpret_cast<const char*>(blob.data()), (std::streamsize)blob.size());
            LogInfo("[TILE_SAVE] Saved to: " + filePath.string() + " bytes=" + std::to_string(blob.size()));
        } else {
            LogError("[TILE_SAVE_FAIL] Failed to write: " + filePath.string());
        }
        LogInfo("[TILE_FETCH] Success: " + url + " size=" + std::to_string(blob.size()) + " bytes");
        job.callback(job.coord, blob);
    } else {
        LogError("[TILE_FETCH_FAIL] Curl error: " + std::string(curl_easy_strerror(res)) + " url=" + url);
    }
}
void OsmTileFetcher::WorkerLoop() {
    while (true) {
        Job job;
        { std::unique_lock<std::mutex> lock(queueMutex); condition.wait(lock, [this] { return stopPool || !jobs.empty(); }); if (stopPool) return; job = jobs.front(); jobs.pop(); }
        ProcessJob(job);
    }
}
void OsmTileFetcher::Fetch(const std::vector<OsmTileCoord>& coords, std::function<void(const OsmTileCoord&, const std::vector<std::byte>&)> callback) {
    { std::unique_lock<std::mutex> lock(queueMutex); for (const auto& coord : coords) jobs.push({coord, callback}); }
    condition.notify_all();
}
