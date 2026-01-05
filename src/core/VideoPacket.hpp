#pragma once
#include <cstdint>
#include <vector>

class VideoPacket {
public:
    std::vector<uint8_t> data;
    bool isKeyFrame = false;
};
