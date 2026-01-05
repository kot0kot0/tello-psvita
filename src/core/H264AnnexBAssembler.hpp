#pragma once
#include <vector>
#include <cstdint>
#include "VideoPacket.hpp"

class H264AnnexBAssembler {
public:
    void push(const uint8_t* data, size_t size);

    bool popAccessUnit(VideoPacket& out);

private:
    std::vector<uint8_t> m_buffer;
    size_t m_parsePos = 0;
    size_t m_analyzedSize = 0;

    std::vector<uint8_t> m_sps;
    std::vector<uint8_t> m_pps;
    std::vector<uint8_t> m_idr;

    static bool isStartCode(const uint8_t* p);
};
