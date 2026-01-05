#include "TelemetryReceiver.hpp"

#include <psp2/net/net.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>

#include <cstdio>
#include <cstring>

static constexpr int TELLO_TELEMETRY_PORT = 8890;
static constexpr int BUF_SIZE = 2048;


static TelemetryState* g_state = nullptr;

void telemetry_receiver_set_state(TelemetryState* state) {
    g_state = state;
}


TelemetryReceiver::TelemetryReceiver()
    : sock_(-1), running_(false) {}

TelemetryReceiver::~TelemetryReceiver() {
    stop();
}

bool TelemetryReceiver::start() {
    if (running_.load()) {
        return true;
    }

    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_ < 0) {
        printf("[telemetry] socket create failed\n");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(TELLO_TELEMETRY_PORT);
    
    if (bind(sock_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        printf("[telemetry] bind failed\n");
        close(sock_);
        sock_ = -1;
        return false;
    }

    running_.store(true);
    thread_ = std::thread(&TelemetryReceiver::recvLoop, this);

    printf("[telemetry] receiver started\n");
    return true;
}

void TelemetryReceiver::stop() {
    running_.store(false);

    if (sock_ >= 0) {
        close(sock_);
        sock_ = -1;
    }

    if (thread_.joinable()) {
        thread_.join();
    }

    printf("[telemetry] receiver stopped\n");
}

void TelemetryReceiver::recvLoop() {
    char buf[BUF_SIZE];

    while (running_.load()) {
        // telemetryは10Hzなので半分sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        ssize_t len = recvfrom(sock_, buf, sizeof(buf) - 1, 0, nullptr, nullptr);
        if (len <= 0) continue;
        buf[len] = '\0';

        if (g_state) {
            auto parse = [&](const char* key) -> int {
                const char* p = strstr(buf, key);
                if (p) return atoi(p + strlen(key));
                return -1;
            };

            // 各値をパースして格納
            int bat = parse("bat:");
            if (bat != -1) g_state->battery.store(bat);

            int h = parse("h:");
            if (h != -1) g_state->h.store(h);

            int tof = parse("tof:");
            if (tof != -1) g_state->tof.store(tof);

            int temph = parse("temph:");
            if (temph != -1) g_state->temph.store(temph);

            // tello eduでwifi強度はtelemetryでは送られてこない
            // // Telloによって wifi: か rssi: か分かれるため両方チェック
            // int wifi = parse("wifi:");
            // if (wifi == -1) wifi = parse("rssi:");
            // if (wifi != -1) g_state->rssi.store(wifi);

            // 速度データ
            g_state->vgx.store(parse("vgx:"));
            g_state->vgy.store(parse("vgy:"));
            g_state->vgz.store(parse("vgz:"));
        }
    } // end while
}