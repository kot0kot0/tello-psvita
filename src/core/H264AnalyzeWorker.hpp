#pragma once
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>

#include "RingBuffer.hpp"
#include "H264AnnexBAssembler.hpp"
#include "VideoPacket.hpp"

class H264AnalyzeWorker {
public:
    H264AnalyzeWorker(RingBuffer& ring);
    ~H264AnalyzeWorker();

    void start();
    void stop();

    bool pop(VideoPacket& out);

private:
    void run();

private:
    RingBuffer& m_ring;
    H264AnnexBAssembler m_assembler;

    std::queue<VideoPacket> m_queue;
    std::mutex m_mutex;

    std::atomic<bool> m_running{false};
    std::thread m_thread;
};
