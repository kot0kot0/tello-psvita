#pragma once

#include <thread>
#include <atomic>

#include "core/TelemetryState.hpp"


void telemetry_receiver_set_state(TelemetryState* state);

class TelemetryReceiver {
public:
    TelemetryReceiver();
    ~TelemetryReceiver();

    bool start();
    void stop();

private:
    void recvLoop();

private:
    int sock_;
    std::atomic<bool> running_;
    std::thread thread_;
};
