#pragma once
#include "ControlState.hpp"
#include <algorithm>

class TelloRcMapper {
public:
    void setGain(float gain) { 
        // 0.2 ~ 1.0に抑える
        // 0.0だと動かなくなってしまい逆に危険
        m_gain = std::min(std::max(gain, 0.2f), 1.0f);; 
    }
    float getGain() const { return m_gain; }
    
    void map(ControlState& io) {
        io.lr  = clamp(static_cast<int>(io.lr  * m_gain));
        io.fb  = clamp(static_cast<int>(io.fb  * m_gain));
        io.ud  = clamp(static_cast<int>(io.ud  * m_gain));
        io.yaw = clamp(static_cast<int>(io.yaw * m_gain));
    }
private:
    float m_gain = 0.5f; // デフォルト50%
    static int clamp(int v) { return (v > 100) ? 100 : (v < -100) ? -100 : v; }
};
