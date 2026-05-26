#include "network/osm_tile_texture.h"
#include "logging.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

OsmTileTexture::OsmTileTexture() {}
OsmTileTexture::~OsmTileTexture() { if (_id != 0) glDeleteTextures(1, &_id); }

bool OsmTileTexture::IsFetching() const { return _isFetching; }

void OsmTileTexture::SetFetching(bool fetching) { 
    std::lock_guard<std::mutex> lock(_texMutex); 
    _isFetching = fetching; 
}

void OsmTileTexture::StbLoad(const std::vector<std::byte>& rawBlob) {
    stbi_set_flip_vertically_on_load(true); 
    int w, h, c;
    const auto ptr = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(rawBlob.data()), (int)rawBlob.size(), &w, &h, &c, STBI_rgb_alpha);
    if (ptr) {
        // OSM тайлы ДОЛЖНЫ быть 256x256 пикселей - это стандарт
        constexpr int EXPECTED_TILE_SIZE = 256;
        
        // Если размер не совпадает - это критическая ошибка, которая ломает отображение карты!
        if (w != EXPECTED_TILE_SIZE || h != EXPECTED_TILE_SIZE) {
            LogError("[TILE_CRITICAL] INVALID TILE SIZE: expected " + std::to_string(EXPECTED_TILE_SIZE) + "x" + std::to_string(EXPECTED_TILE_SIZE) + 
                   ", got " + std::to_string(w) + "x" + std::to_string(h) + ". TILE WILL BE SKIPPED to prevent rendering artifacts.");
            stbi_image_free(ptr);
            return; // НЕ загружаем некорректный тайл!
        }
        
        const size_t nbytes = size_t(w * h * STBI_rgb_alpha);
        std::lock_guard<std::mutex> lock(_texMutex);
        _width = w; _height = h; _channels = c;
        _rgbaBlob.assign(reinterpret_cast<std::byte*>(ptr), reinterpret_cast<std::byte*>(ptr) + nbytes);
        _hasNewData = true; _isFetching = false; 
        stbi_image_free(ptr);
        LogInfo("[TILE_OK] StbLoad: " + std::to_string(w) + "x" + std::to_string(h) + " bytes=" + std::to_string(nbytes));
    } else {
        LogError("[TILE_ERROR] stbi_load_from_memory failed for blob size=" + std::to_string(rawBlob.size()));
    }
}

void OsmTileTexture::GlLoad() {
    std::lock_guard<std::mutex> lock(_texMutex); 
    if (!_hasNewData) return;
    
    // Дополнительная проверка перед загрузкой в OpenGL: тайл должен быть 256x256
    constexpr int EXPECTED_TILE_SIZE = 256;
    if (_width != EXPECTED_TILE_SIZE || _height != EXPECTED_TILE_SIZE) {
        LogError("[TILE_GL_SKIP] Texture size mismatch before GlLoad: " + std::to_string(_width) + "x" + std::to_string(_height) + 
                ". Expected " + std::to_string(EXPECTED_TILE_SIZE) + "x" + std::to_string(EXPECTED_TILE_SIZE));
        return;
    }
    
    if (_id == 0) glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, _rgbaBlob.data());
    _hasNewData = false;
    LogInfo("[TILE_GL] Loaded id=" + std::to_string(_id) + " size=" + std::to_string(_width) + "x" + std::to_string(_height));
}

GLuint OsmTileTexture::GetId() const { return _id; }