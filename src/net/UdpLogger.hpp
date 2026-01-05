#pragma once
#include <string>
#include <netinet/in.h>

class UdpLogger {
public:
    static UdpLogger& getInstance() {
        static UdpLogger instance;
        return instance;
    }

    void init(const char* ip, int port);
    void log(const char* fmt, ...);

private:
    UdpLogger() : m_sock(-1) {}
    int m_sock;
    sockaddr_in m_addr;
};