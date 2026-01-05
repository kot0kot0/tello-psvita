#include "UdpCommandSender.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

UdpCommandSender::UdpCommandSender(const char* ip, int port) {
    m_sock = socket(AF_INET, SOCK_DGRAM, 0);

    m_addr = {};
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip);
}

UdpCommandSender::~UdpCommandSender() {
    close(m_sock);
}

void UdpCommandSender::send(const std::string& cmd) {
    sendto(
        m_sock,
        cmd.c_str(),
        cmd.size(),
        0,
        (sockaddr*)&m_addr,
        sizeof(m_addr)
    );
}

std::string UdpCommandSender::sendAndWaitResponse(const std::string& cmd, int timeout_ms) {
    // 1. コマンド送信
    send(cmd);

    // 2. 受信タイムアウトの設定
    // Vita/PSP2のネットワークスタックに合わせたタイムアウト設定
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;
    setsockopt(m_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    // 3. レスポンスの受信
    char buf[128];
    sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    int len = recvfrom(
        m_sock,
        buf,
        sizeof(buf) - 1,
        0,
        (sockaddr*)&from_addr,
        &from_len
    );

    if (len > 0) {
        buf[len] = '\0';
        return std::string(buf);
    }

    return ""; // タイムアウト、またはエラー時は空文字を返す
}