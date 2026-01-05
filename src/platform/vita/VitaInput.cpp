#include "VitaInput.hpp"
#include <psp2/ctrl.h>

static int normalize_with_deadzone(int v) {
    v -= 128; // -128 to 127
    int val = v * 100 / 128;
    
    // デッドゾーン設定 (10%以下の入力は無視)
    if (val > -10 && val < 10) return 0;
    
    // 10%以上の入力を 0〜100 にリマップするとよりスムーズ
    if (val >= 10) return (val - 10) * 100 / 90;
    if (val <= -10) return (val + 10) * 100 / 90;
    return 0;
}

void VitaInput::poll(ControlState& out, uint64_t delta_ms) {
    SceCtrlData pad{};
    sceCtrlPeekBufferPositive(0, &pad, 1);

    // スティック入力
    out.ud  = -normalize_with_deadzone(pad.ly);
    out.yaw =  normalize_with_deadzone(pad.lx);
    out.fb  = -normalize_with_deadzone(pad.ry);
    out.lr  =  normalize_with_deadzone(pad.rx);

    // ○△×□入力
    auto update_hold = [&](bool button_pressed, int& timer, bool& trigger) {
        if (button_pressed) {
            timer += delta_ms;
            if (timer >= 1500) { trigger = true; timer = 0; }
        } else {
            timer = 0;
            trigger = false;
        }
    };
    update_hold((pad.buttons & SCE_CTRL_TRIANGLE), m_takeOffTimer, out.takeOffTrigger);
    update_hold((pad.buttons & SCE_CTRL_CROSS),    m_landTimer,    out.landTrigger);
    update_hold((pad.buttons & SCE_CTRL_SQUARE),   m_flipLTimer,   out.flipLTrigger);
    update_hold((pad.buttons & SCE_CTRL_CIRCLE),   m_flipRTimer,   out.flipRTrigger);

    // 左の十字キー入力
    static bool prev_up = false, prev_down = false;
    bool curr_up = (pad.buttons & SCE_CTRL_UP);
    bool curr_down = (pad.buttons & SCE_CTRL_DOWN);
    if (curr_up && !prev_up) {
        out.gainUpTrigger = true;
    }
    if (curr_down && !prev_down) {
        out.gainDownTrigger = true;
    }
    prev_up = curr_up;
    prev_down = curr_down;

    // l,r入力
    // 単押しトリガー（撮影・録画）
    static bool prev_r = false, prev_l = false;
    bool curr_r = (pad.buttons & SCE_CTRL_RTRIGGER);
    bool curr_l = (pad.buttons & SCE_CTRL_LTRIGGER);
    // 「前回押されていなくて、今押されている」瞬間を検知
    out.photoTrigger = (curr_r && !prev_r);
    out.recordToggle = (curr_l && !prev_l);
    prev_r = curr_r;
    prev_l = curr_l;

    // START,SELECT入力
    // 緊急停止 (START + SELECT 同時押し)
    out.emergencyTrigger = (pad.buttons & SCE_CTRL_START) && (pad.buttons & SCE_CTRL_SELECT);
    // menu表示(SELECTのみ)
    out.menu = (pad.buttons & SCE_CTRL_SELECT) && !out.emergencyTrigger;
}