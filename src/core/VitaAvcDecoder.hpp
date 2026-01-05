#include "core/IVideoDecoder.hpp"

#include <psp2/videodec.h>


class VitaAvcDecoder : public IVideoDecoder {
public:
    VitaAvcDecoder() {}
    ~VitaAvcDecoder() {}

    bool initialize(int width, int height) override;
    bool decode(const VideoPacket& pkt, void* outTexturePtr, size_t stride) override;

private:
    SceAvcdecCtrl* m_decoder = nullptr;
    SceUID m_memblock = -1;
    int m_width = 0;
    int m_height = 0;
};