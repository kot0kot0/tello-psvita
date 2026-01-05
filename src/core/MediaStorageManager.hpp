#pragma once
#include <string>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>

#include <vita2d.h>
#include <psp2/kernel/threadmgr.h>

class MediaStorageManager {
public:
    MediaStorageManager(const std::string& baseDir);
    ~MediaStorageManager();

    // 写真保存リクエスト (テクスチャを渡すだけであとはお任せ)
    void requestSnapshot(vita2d_texture* tex);

    // 録画の開始・停止切り替え
    void toggleRecording();
    
    // 録画用パケットの投入
    void pushVideoPacket(const std::vector<uint8_t>& data);

    // 状態取得 (UI表示用)
    bool isRecording() const { return m_isRecording; }
    bool isPreparing() const { return m_isPreparing; }

private:
    // 内部構造体
    struct PhotoRequest {
        std::vector<uint8_t> pixelData;
        std::string filename;
        int width, height, stride;
    };

    struct VideoRequest {
        std::vector<uint8_t> data;
    };

    // ワーカースレッド（static関数として定義）
    static int photoWorkerStatic(SceSize args, void *argp);
    static int videoWorkerStatic(SceSize args, void *argp);
    
    void photoWorkerLoop();
    void videoWorkerLoop();

    // 共通ユーティリティ
    std::string generateTimestampFilename(const std::string& prefix, const std::string& ext);

    std::string m_baseDir;

    // 写真用
    std::queue<PhotoRequest> m_photoQueue;
    std::mutex m_photoMutex;
    std::thread m_photoThreadHandle;
    
    // 録画用
    std::queue<VideoRequest> m_videoQueue;
    std::mutex m_videoMutex;
    std::thread m_videoThreadHandle;

    std::atomic<bool> m_isRecording{false};
    std::atomic<bool> m_isPreparing{false};
    std::atomic<bool> m_waitingForIFrame{true};
    FILE* m_videoFile = nullptr;
    
    std::atomic<bool> m_running{true}; // スレッド終了フラグ
};