#include "UdpReceiver.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

UdpReceiver::UdpReceiver(RingBuffer& buffer, int port)
    : m_buffer(buffer), m_port(port), m_running(false) {}

UdpReceiver::~UdpReceiver() {
    stop();
}

void UdpReceiver::start() {
    m_running = true;
    m_thread = std::thread(&UdpReceiver::run, this);
}

void UdpReceiver::stop() {
    m_running = false;
    if (m_thread.joinable()) m_thread.join();
    if (m_socket >= 0) close(m_socket);
}

void UdpReceiver::run() {
    m_socket = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(m_socket, (sockaddr*)&addr, sizeof(addr));

    uint8_t buf[3000];
    while (m_running) {
        ssize_t len = recvfrom(m_socket, buf, sizeof(buf), 0, NULL, NULL);
        if (len > 0) {
            m_buffer.write(buf, len);
        }
        // std::this_thread::sleep_for(std::chrono::microseconds(500));
    }
}
