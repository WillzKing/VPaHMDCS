#pragma once
#include <GL/glew.h>
#include <cstddef>
#include <mutex>
#include <vector>

class OsmTileTexture {
public:
    OsmTileTexture();
    ~OsmTileTexture();
    bool IsFetching() const;
    void SetFetching(bool fetching);
    void StbLoad(const std::vector<std::byte>& rawBlob);
    void GlLoad();
    GLuint GetId() const;

private:
    mutable std::mutex _texMutex;
    GLuint _id{0};
    int _width{0}, _height{0}, _channels{0};
    bool _hasNewData{false};
    bool _isFetching{false};
    std::vector<std::byte> _rgbaBlob;
};