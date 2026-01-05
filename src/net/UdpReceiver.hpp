#pragma once
#include <thread>
#include <atomic>
#include "core/RingBuffer.hpp"

class UdpReceiver {
public:
    UdpReceiver(RingBuffer& buffer, int port);
    ~UdpReceiver();

    void start();
    void stop();

private:
    void run();

    RingBuffer& m_buffer;
    int m_port;
    int m_socket;
    std::thread m_thread;
    std::atomic<bool> m_running;
};
