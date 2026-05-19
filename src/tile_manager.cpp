#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define byte win_byte_override
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "tile_manager.h"
#include "curl_funcs.h"
#include <stb_image.h>
#include <GL/glew.h>
#include <filesystem>
#include <cstdio>
#include <iostream>
namespace fs = std::filesystem;
using namespace std;

TileManager::TileManager(const string& cacheDir) : cacheDir_(cacheDir) {
    fs::create_directories(cacheDir_);
    workerThread_ = thread(&TileManager::workerThreadFunc, this);
}

TileManager::~TileManager() { stop(); }

string TileManager::makeKey(int z, int x, int y) const {
    return to_string(z) + "/" + to_string(x) + "/" + to_string(y);
}

string TileManager::getTilePath(int z, int x, int y) const {
    return cacheDir_ + "/" + makeKey(z, x, y) + ".png";
}

void TileManager::requestTile(int z, int x, int y) {
    string id = makeKey(z, x, y);
    lock_guard lock(cacheMutex_);
    if (textures_.find(id) == textures_.end()) {
        textures_[id].isLoading = true;
        { lock_guard qLock(queueMutex_); taskQueue_.push(id); }
        queueCV_.notify_one();
    }
}

TileTexture* TileManager::getTile(int z, int x, int y) {
    string id = makeKey(z, x, y);
    lock_guard lock(cacheMutex_);
    auto it = textures_.find(id);
    if (it != textures_.end() && it->second.textureId != 0) return &it->second;
    return nullptr;
}

void TileManager::update() {
    lock_guard lock(cacheMutex_);
    for (auto& [id, tex] : textures_) {
        if (!tex.rgbaBlob.empty() && tex.textureId == 0) {
            glGenTextures(1, &tex.textureId);
            glBindTexture(GL_TEXTURE_2D, tex.textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.rgbaBlob.data());
            tex.rgbaBlob.clear();
            tex.isLoading = false;
        }
    }
}

void TileManager::workerThreadFunc() {
    while (running_) {
        string tileId;
        {
            unique_lock lock(queueMutex_);
            queueCV_.wait_for(lock, chrono::milliseconds(100), [this] { return !taskQueue_.empty() || !running_; });
            if (!running_) break;
            if (taskQueue_.empty()) continue;
            tileId = taskQueue_.front(); taskQueue_.pop();
        }
        
        int z, x, y;
        sscanf(tileId.c_str(), "%d/%d/%d", &z, &x, &y);
        
        vector<uint8_t> data;
        string path = getTilePath(z, x, y);
        
        if (!fs::exists(path)) {
            if (CurlFuncs::downloadTile(z, x, y, data)) CurlFuncs::saveToFile(path, data);
        } else { CurlFuncs::loadFromFile(path, data); }
        
        if (!data.empty()) {
            int w, h, ch;
            uint8_t* img = stbi_load_from_memory(data.data(), (int)data.size(), &w, &h, &ch, 4);
            if (img) {
                lock_guard lock(cacheMutex_);
                textures_[tileId].rgbaBlob.assign(img, img + (w * h * 4));
                textures_[tileId].width = w; textures_[tileId].height = h;
                stbi_image_free(img);
            }
        }
    }
}

void TileManager::clearQueue() {
    lock_guard lock(queueMutex_);
    queue<string> empty; swap(taskQueue_, empty);
}

void TileManager::clearCache() {
    lock_guard lock(cacheMutex_);
    for (auto& [id, tex] : textures_) {
        if (tex.textureId != 0) {
            glDeleteTextures(1, &tex.textureId);
            tex.textureId = 0;
        }
        tex.rgbaBlob.clear();
        tex.isLoading = false;
        tex.valid = false;
    }
    textures_.clear();
    cout << "[TileManager] Кэш очищен" << endl;
}

size_t TileManager::getCacheSize() const {
    lock_guard lock(cacheMutex_);
    return textures_.size();
}

void TileManager::stop() {
    running_ = false; queueCV_.notify_all();
    if (workerThread_.joinable()) workerThread_.join();
}