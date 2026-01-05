#pragma once
#include <cstdint>

class ControlState {
public:
    // -100 ～ 100（論理値）
    int lr = 0;   // left / right
    int fb = 0;   // forward / backward
    int ud = 0;   // up / down
    int yaw = 0;  // yaw

    bool emergencyTrigger = false; // 緊急停止
    bool landTrigger = false; // 着陸
    bool takeOffTrigger = false; // 離陸
    bool flipLTrigger = false; // 左方向にflip回転する
    bool flipRTrigger = false; // 左方向にflip回転する
    
    bool photoTrigger = false;   // 写真ボタンが押された瞬間
    bool recordToggle = false;  // 録画ボタンが押された瞬間
    bool isRecording = false;   // 現在録画中かどうかの状態保持

    bool gainUpTrigger = false; // 移動速度Up
    bool gainDownTrigger = false; // 移動速度Down

    bool menu = false;
};
