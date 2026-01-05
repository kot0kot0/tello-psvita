#pragma once
#include <cstdint>
#include <vector>

#include "VideoPacket.hpp"

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;
    virtual bool initialize(int width, int height) = 0;
    virtual bool decode(const VideoPacket& pkt, void* outTexturePtr, size_t stride) = 0;
};
