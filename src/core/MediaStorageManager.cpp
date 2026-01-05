#include "MediaStorageManager.hpp"
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <jpeglib.h>
#include <psp2/io/stat.h>

MediaStorageManager::MediaStorageManager(const std::string& baseDir) 
    : m_baseDir(baseDir), m_running(true), m_isRecording(false), m_isPreparing(false), m_videoFile(nullptr)
{
    // 保存先ディレクトリの作成
    sceIoMkdir(m_baseDir.c_str(), 0777);

    // 写真保存スレッドの起動
    m_photoThreadHandle = std::thread([this]() { photoWorkerLoop(); });
    // 録画スレッドの起動
    m_videoThreadHandle = std::thread([this]() { videoWorkerLoop(); });
}

MediaStorageManager::~MediaStorageManager() {
    m_running = false;
    
    // スレッドの終了を待機
    if (m_photoThreadHandle.joinable()) m_photoThreadHandle.join();
    if (m_videoThreadHandle.joinable()) m_videoThreadHandle.join();
    
    if (m_videoFile) {
        fclose(m_videoFile);
        m_videoFile = nullptr;
    }
}

void MediaStorageManager::requestSnapshot(vita2d_texture* tex) {
    if (!tex) return;

    PhotoRequest req;
    req.width = vita2d_texture_get_width(tex);
    req.height = vita2d_texture_get_height(tex);
    req.stride = vita2d_texture_get_stride(tex);
    req.filename = generateTimestampFilename("photo_", ".jpg");

    // メモリコピー（スレッドに渡すため）
    req.pixelData.resize(req.stride * req.height);
    memcpy(req.pixelData.data(), vita2d_texture_get_datap(tex), req.pixelData.size());

    std::lock_guard<std::mutex> lock(m_photoMutex);
    m_photoQueue.push(std::move(req));
}

void MediaStorageManager::toggleRecording() {
    if (!m_isRecording && !m_isPreparing) {
        // 録画開始
        std::string path = generateTimestampFilename("video_", ".h264");
        m_videoFile = fopen(path.c_str(), "wb");
        if (m_videoFile) {
            m_waitingForIFrame = true;
            m_isPreparing = true; // SPS待ち状態
        }
    } else {
        // 録画停止
        m_isRecording = false;
        m_isPreparing = false;
        // スレッド側でファイルが閉じられるのを安全に待つための遅延は不要（atomicで制御）
        // 実際にはスレッド側で nullptr チェックをする
    }
}

void MediaStorageManager::pushVideoPacket(const std::vector<uint8_t>& data) {
    if (!m_isRecording && !m_isPreparing) return;

    VideoRequest req;
    req.data = data;
    std::lock_guard<std::mutex> lock(m_videoMutex);
    m_videoQueue.push(std::move(req));
}

// --- スレッドブリッジ ---
int MediaStorageManager::photoWorkerStatic(SceSize args, void *argp) {
    MediaStorageManager* instance = *(MediaStorageManager**)argp;
    instance->photoWorkerLoop();
    return 0;
}

int MediaStorageManager::videoWorkerStatic(SceSize args, void *argp) {
    MediaStorageManager* instance = *(MediaStorageManager**)argp;
    instance->videoWorkerLoop();
    return 0;
}

void MediaStorageManager::photoWorkerLoop() {
    while (m_running) {
        PhotoRequest req;
        bool hasJob = false;

        {
            std::lock_guard<std::mutex> lock(m_photoMutex);
            if (!m_photoQueue.empty()) {
                req = std::move(m_photoQueue.front());
                m_photoQueue.pop();
                hasJob = true;
            }
        }

        if (hasJob) {
            FILE* outfile = fopen(req.filename.c_str(), "wb");
            if (outfile) {
                struct jpeg_compress_struct cinfo;
                struct jpeg_error_mgr jerr;
                cinfo.err = jpeg_std_error(&jerr);
                jpeg_create_compress(&cinfo);
                jpeg_stdio_dest(&cinfo, outfile);

                cinfo.image_width = req.width;
                cinfo.image_height = req.height;
                cinfo.input_components = 3;
                cinfo.in_color_space = JCS_RGB;

                jpeg_set_defaults(&cinfo);
                jpeg_set_quality(&cinfo, 90, TRUE);
                jpeg_start_compress(&cinfo, TRUE);

                unsigned char* row_pointer = (unsigned char*)malloc(req.width * 3);
                while (cinfo.next_scanline < cinfo.image_height) {
                    unsigned char* src = req.pixelData.data() + (cinfo.next_scanline * req.stride);
                    for (int x = 0; x < req.width; x++) {
                        row_pointer[x * 3 + 0] = src[x * 4 + 0]; // R
                        row_pointer[x * 3 + 1] = src[x * 4 + 1]; // G
                        row_pointer[x * 3 + 2] = src[x * 4 + 2]; // B
                    }
                    jpeg_write_scanlines(&cinfo, &row_pointer, 1);
                }
                jpeg_finish_compress(&cinfo);
                fclose(outfile);
                jpeg_destroy_compress(&cinfo);
                free(row_pointer);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void MediaStorageManager::videoWorkerLoop() {
    while (m_running) {
        VideoRequest req;
        bool hasJob = false;

        {
            std::lock_guard<std::mutex> lock(m_videoMutex);
            if (!m_videoQueue.empty()) {
                req = std::move(m_videoQueue.front());
                m_videoQueue.pop();
                hasJob = true;
            }
        }

        if (hasJob && m_videoFile) {
            if (m_waitingForIFrame) {
                // SPS (NAL Type 7) を探す
                if (req.data.size() >= 5 && req.data[0] == 0x00 && req.data[1] == 0x00 && 
                    req.data[2] == 0x00 && req.data[3] == 0x01) {
                    if ((req.data[4] & 0x1F) == 7) {
                        m_waitingForIFrame = false;
                        m_isPreparing = false;
                        m_isRecording = true;
                    }
                }
            }

            if (m_isRecording) {
                fwrite(req.data.data(), 1, req.data.size(), m_videoFile);
            }
        } else {
            // 録画中でない場合はファイルを閉じる（安全なクローズ処理）
            if (!m_isRecording && !m_isPreparing && m_videoFile) {
                fclose(m_videoFile);
                m_videoFile = nullptr;
                // 残ったキューをクリア
                std::lock_guard<std::mutex> lock(m_videoMutex);
                while(!m_videoQueue.empty()) m_videoQueue.pop();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

std::string MediaStorageManager::generateTimestampFilename(const std::string& prefix, const std::string& ext) {
    std::time_t t = std::time(nullptr);
    char timestamp[32];
    std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", std::localtime(&t));
    return m_baseDir + prefix + timestamp + ext;
}