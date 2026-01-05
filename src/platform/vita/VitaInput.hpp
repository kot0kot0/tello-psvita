#pragma once
#include "core/ControlState.hpp"
#include <cstdint>

class VitaInput {
public:
    VitaInput() : m_takeOffTimer(0), m_landTimer(0), m_flipLTimer(0), m_flipRTimer(0) {} 

    void poll(ControlState& out, uint64_t delta_ms);

private:
    // ボタン長押し判定用の累積タイマー(ms)
    int m_takeOffTimer;
    int m_landTimer;
    int m_flipLTimer;
    int m_flipRTimer;
};