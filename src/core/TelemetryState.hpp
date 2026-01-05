#pragma once
#include <atomic>

struct TelemetryState {
    std::atomic<int> battery{0};
    std::atomic<int> h{0};      // 気圧高度(cm)
    std::atomic<int> tof{0};    // TOF距離(cm)
    std::atomic<int> temph{0};  // 最高温度(度)
    std::atomic<int> rssi{0};   // Wi-Fi強度（取れた場合のみ）
    std::atomic<int> vgx{0}, vgy{0}, vgz{0};
};