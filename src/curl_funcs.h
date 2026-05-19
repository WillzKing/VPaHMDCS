#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <curl/curl.h>
using namespace std;

class CurlFuncs {
public:
    static bool init();
    static void cleanup();
    static bool downloadTile(int zoom, int x, int y, vector<uint8_t>& outData);
    static bool saveToFile(const string& path, const vector<uint8_t>& data);
    static bool loadFromFile(const string& path, vector<uint8_t>& outData);
private:
    static size_t writeCallback(void* data, size_t size, size_t nmemb, void* userp);
};