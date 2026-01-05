#include "H264AnalyzeWorker.hpp"
#include <psp2/kernel/threadmgr.h>

H264AnalyzeWorker::H264AnalyzeWorker(RingBuffer& ring)
    : m_ring(ring) {}

H264AnalyzeWorker::~H264AnalyzeWorker() {
    stop();
}

void H264AnalyzeWorker::start() {
    if (m_running.load()) return;
    m_running = true;
    m_thread = std::thread(&H264AnalyzeWorker::run, this);
}

void H264AnalyzeWorker::stop() {
    m_running = false;
    if (m_thread.joinable())
        m_thread.join();
}

void H264AnalyzeWorker::run() {
    std::vector<uint8_t> tmp(65535); // ヒープへ
    VideoPacket pkt;

    while (m_running.load()) {
        // 最新データを取得
        size_t r = m_ring.read(tmp.data(), tmp.size());
        // 最新データをh264のバッファに追加
        if (r > 0)
            m_assembler.push(tmp.data(), r);

        // 1packet分の取得を試みる
        while (m_assembler.popAccessUnit(pkt)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(pkt)); // copyではなくmove
        }

        // 解析は処理時間がかかるのでsleepは短めにする
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}

bool H264AnalyzeWorker::pop(VideoPacket& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queue.empty())
        return false;
    out = m_queue.front();
    m_queue.pop();
    return true;
}
