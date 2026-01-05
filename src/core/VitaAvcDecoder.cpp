#include "VitaAvcDecoder.hpp"
#include <cstdlib>
#include <cstring>

#include <vita2d.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/sysmem.h>

bool VitaAvcDecoder::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    int ret; // 初期化結果確認用

    // ---- Video Decoder Init ----
    SceVideodecQueryInitInfoHwAvcdec init{};
    init.size = sizeof(init);
    init.horizontal = width;
    init.vertical   = height;
    init.numOfRefFrames = 1;
    init.numOfStreams   = 1;

    ret = sceVideodecInitLibrary(
        SCE_VIDEODEC_TYPE_HW_AVCDEC,
        &init
    );
    if (ret < 0) {
        return false;
    }

    SceAvcdecQueryDecoderInfo q{};
    q.horizontal = init.horizontal;
    q.vertical   = init.vertical;
    q.numOfRefFrames = init.numOfRefFrames;

    SceAvcdecDecoderInfo info{};
    ret = sceAvcdecQueryDecoderMemSize(
        SCE_VIDEODEC_TYPE_HW_AVCDEC,
        &q,
        &info
    );
    if (ret < 0) {
        return false;
    }

    m_decoder = (SceAvcdecCtrl*)calloc(1, sizeof(SceAvcdecCtrl));

    // 1MB アライン
    size_t frameBufSize = (info.frameMemSize + 0xFFFFF) & ~0xFFFFF;
    m_decoder->frameBuf.size = frameBufSize;

    m_memblock = sceKernelAllocMemBlock(
        "decoder",
        SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW,
        frameBufSize,
        NULL
    );
    if (m_memblock < 0) {
        return false;
    }

    sceKernelGetMemBlockBase(m_memblock, &m_decoder->frameBuf.pBuf);

    ret = sceAvcdecCreateDecoder(SCE_VIDEODEC_TYPE_HW_AVCDEC, m_decoder, &q);
    if (ret < 0) {
        return false;
    }

    return true;
}

bool VitaAvcDecoder::decode(const VideoPacket& pkt, void* outTexturePtr, size_t stride) {
    if (!m_decoder) return false;

    SceAvcdecAu au{};
    SceAvcdecArrayPicture array_picture{};
    SceAvcdecPicture picture{};
    SceAvcdecPicture* pictures = &picture;

    array_picture.numOfElm = 1;
    array_picture.pPicture = &pictures;

    picture.size = sizeof(picture);
    picture.frame.pixelType   = SCE_AVCDEC_PIXELFORMAT_RGBA8888; // vitaのtextureに合わせる
    // H.264に合わせる
    picture.frame.frameWidth  = m_width;
    picture.frame.frameHeight = m_height;
    // textureに合わせる
    picture.frame.framePitch  = stride / 4; // バイト数をピクセル数に変換
    picture.frame.pPicture[0] = outTexturePtr;

    au.es.pBuf  = const_cast<uint8_t*>(pkt.data.data());
    au.es.size  = pkt.data.size();
    au.dts.lower = au.dts.upper = 0xFFFFFFFF;
    au.pts.lower = au.pts.upper = 0xFFFFFFFF;

    return sceAvcdecDecode(m_decoder, &au, &array_picture) >= 0;
}