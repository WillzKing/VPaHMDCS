#include "curl_funcs.h"
#include <fstream>
#include <iostream>
#include <filesystem>
namespace fs = std::filesystem;
using namespace std;

bool CurlFuncs::init() {
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    cout << "[CURL] curl_global_init: " << (res == CURLE_OK ? "OK" : "ERROR") << " (code: " << res << ")\n";
    return res == CURLE_OK;
}

void CurlFuncs::cleanup() { cout << "[CURL] curl_global_cleanup\n"; curl_global_cleanup(); }

size_t CurlFuncs::writeCallback(void* data, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    auto& blob = *static_cast<vector<uint8_t>*>(userp);
    auto const* dataptr = static_cast<uint8_t*>(data);
    blob.insert(blob.cend(), dataptr, dataptr + realsize);
    return realsize;
}

bool CurlFuncs::downloadTile(int zoom, int x, int y, vector<uint8_t>& outData) {
    CURL* curl = curl_easy_init();
    if (!curl) { cerr << "[CURL] curl_easy_init вернул nullptr!\n"; return false; }
    string url = "https://tile.openstreetmap.org/" + to_string(zoom) + "/" + to_string(x) + "/" + to_string(y) + ".png";
    cout << "[CURL] Запрос: " << url << "\n";
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "BackendControl/1.0 (+https://github.com/dokbrawn/vis-backend.git; contact: tanisimov2006@gmail.com)");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &outData);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode res = curl_easy_perform(curl);
    long responseCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &responseCode);
    size_t receivedSize = outData.size();
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) { cerr << "[CURL] Ошибка: " << curl_easy_strerror(res) << " (code: " << res << ")\n"; outData.clear(); return false; }
    if (responseCode != 200) { cerr << "[CURL] HTTP " << responseCode << " для " << url << "\n"; outData.clear(); return false; }
    cout << "[CURL] Успешно: " << receivedSize << " байт\n";
    return true;
}

bool CurlFuncs::saveToFile(const string& path, const vector<uint8_t>& data) {
    try {
        fs::path p(path); fs::create_directories(p.parent_path());
        ofstream file(path, ios::binary);
        if (!file) { cerr << "[CURL] Не открыть файл: " << path << "\n"; return false; }
        file.write(reinterpret_cast<const char*>(data.data()), data.size());
        file.close();
        cout << "[CURL] Сохранено: " << path << " (" << data.size() << " байт)\n";
        return true;
    } catch (const exception& e) { cerr << "[CURL] Исключение: " << e.what() << "\n"; return false; }
}

bool CurlFuncs::loadFromFile(const string& path, vector<uint8_t>& outData) {
    try {
        ifstream file(path, ios::binary | ios::ate);
        if (!file) { cerr << "[CURL] Не открыть файл: " << path << "\n"; return false; }
        streamsize size = file.tellg(); file.seekg(0, ios::beg);
        outData.resize(size);
        if (!file.read(reinterpret_cast<char*>(outData.data()), size)) { cerr << "[CURL] Ошибка чтения: " << path << "\n"; outData.clear(); return false; }
        cout << "[CURL] Загружено: " << path << " (" << size << " байт)\n";
        return true;
    } catch (const exception& e) { cerr << "[CURL] Исключение: " << e.what() << "\n"; return false; }
}