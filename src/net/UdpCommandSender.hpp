#pragma once
#include <string>
#include <netinet/in.h>

class UdpCommandSender {
public:
    UdpCommandSender(const char* ip, int port);
    ~UdpCommandSender();

    // 従来の送りっぱなしメソッド
    void send(const std::string& cmd);

    // 送信してレスポンス（"ok"や"90"など）を受け取るメソッド
    std::string sendAndWaitResponse(const std::string& cmd, int timeout_ms = 100);

private:
    int m_sock;
    sockaddr_in m_addr;
};