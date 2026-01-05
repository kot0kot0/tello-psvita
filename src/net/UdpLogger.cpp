#include "UdpLogger.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdio>
#include <cstdarg>

void UdpLogger::init(const char* ip, int port) {
    m_sock = socket(AF_INET, SOCK_DGRAM, 0);
    m_addr = {};
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip);
}

void UdpLogger::log(const char* fmt, ...) {
    if (m_sock < 0) return;

    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    sendto(m_sock, buf, len, 0, (sockaddr*)&m_addr, sizeof(m_addr));
}