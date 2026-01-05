#include <vita2d.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/sysmodule.h>
#include <psp2/videodec.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/stat.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <jpeglib.h>
#include <ctime>
#include <queue>
#include <mutex>
#include <vector>

#include "core/RingBuffer.hpp"
#include "core/H264AnalyzeWorker.hpp"
#include "core/TelemetryState.hpp"
#include "core/VitaAvcDecoder.hpp"
#include "core/MediaStorageManager.hpp"

#include "net/UdpReceiver.hpp"
#include "net/UdpCommandSender.hpp"
#include "net/TelemetryReceiver.hpp"
#include "net/TelloCommand.hpp"
#include "net/UdpLogger.hpp"

#include "platform/vita/VitaInput.hpp"
#include "core/TelloRcMapper.hpp"

// 物理的なデバイス解像度
static constexpr int PSVITA_SCREEN_WIDTH  = 960;
static constexpr int PSVITA_SCREEN_HEIGHT = 544;

// Telloから送られてくる映像の論理解像度
static constexpr int TELLO_VIDEO_WIDTH    = 960;
static constexpr int TELLO_VIDEO_HEIGHT   = 720;

#include "font.h"
extern unsigned int basicfont_size;
extern unsigned char basicfont[];

TelemetryState g_telemetry;



int main() {
    // 撮影/録画クラスを初期化
    MediaStorageManager storage("ux0:data/TelloPSVita/");

    // ---- Input ----
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

    // ---- Network ----
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    SceNetInitParam netInitParam{};
    void* netMem = malloc(1 * 1024 * 1024);
    netInitParam.memory = netMem;
    netInitParam.size   = 1 * 1024 * 1024;
    netInitParam.flags = 0;
    sceNetInit(&netInitParam);

    // ---- vita2d ----
    vita2d_init();
    vita2d_font* font = vita2d_load_font_mem(basicfont, basicfont_size);
    if (!font) {
        while (1) {}
    }

    // ---- hardware decoder ----
    VitaAvcDecoder decoder;
    decoder.initialize(TELLO_VIDEO_WIDTH, TELLO_VIDEO_HEIGHT);
    vita2d_texture* frame_texture = vita2d_create_empty_texture_format(
                                        TELLO_VIDEO_WIDTH, 
                                        TELLO_VIDEO_HEIGHT, 
                                        SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_ABGR
                                    );
    if (!frame_texture) {
        return -1;
    }


    // ---- Video Receive ----
    RingBuffer ring(4 * 1024 * 1024);
    UdpReceiver h264Receiver(ring, 11111);
    H264AnalyzeWorker h264Analyzer(ring);
    h264Receiver.start();
    h264Analyzer.start();
    
    // ---- Telemetry Receive ----
    TelemetryReceiver telemetry;
    telemetry_receiver_set_state(&g_telemetry);
    telemetry.start();


    // ---- Control ----
    VitaInput input;
    TelloRcMapper mapper;
    UdpCommandSender sender("192.168.10.1", 8889);

    sender.send("command");
    sceKernelDelayThread(1000 * 1000);
    sender.send("streamon");
    sceKernelDelayThread(1000 * 1000);

    
    // 時間管理用の変数
    auto last_rc_time = std::chrono::steady_clock::now();
    auto last_wifi_time = std::chrono::steady_clock::now();
    auto last_fps_time = std::chrono::steady_clock::now();
    int frame_count = 0;
    int current_fps = 0;
    
    // --- ループ外で録画状態を管理 ---
    bool is_recording = false; // 撮影中かのフラグ
    int flash_alpha = 0; // 不透明度で撮影を表現

    // 制御状態
    // 押した瞬間などを表現するためwhileループの外で宣言
    ControlState ctrl;


    // UdpLogger
    UdpLogger::getInstance().init("192.168.11.8", 12345);
    float display_gain_timer = 0; // Gain表示用のタイマー



    // ================= MAIN LOOP =================
    while (true) {
        auto now = std::chrono::steady_clock::now();

        // 1. 【RCコマンド送信】 50ms (秒間20回) おきに固定
        auto rc_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_rc_time).count();
        if (rc_elapsed_ms >= 50) { 
            input.poll(ctrl, rc_elapsed_ms); // 前回からの経過時間を渡す

            // --- Gain調整 (送信ではないので独立して実行) ---
            if (ctrl.gainUpTrigger) {
                mapper.setGain(mapper.getGain() + 0.1f);
                display_gain_timer = 2000.0f; // 2秒間表示
                UdpLogger::getInstance().log("[Info] Gain Up: %.1f", mapper.getGain());
                ctrl.gainUpTrigger = false;
            }
            if (ctrl.gainDownTrigger) {
                mapper.setGain(mapper.getGain() - 0.1f);
                display_gain_timer = 2000.0f; // 2秒間表示
                UdpLogger::getInstance().log("[Info] Gain Down: %.1f", mapper.getGain());
                ctrl.gainDownTrigger = false;
            }
            
            // --- 撮影・録画 (送信ではないので独立して実行) ---
            if (ctrl.photoTrigger) {
                storage.requestSnapshot(frame_texture); // 撮影を要求
                // フラッシュの不透明度を最大にセット
                flash_alpha = 150;
                ctrl.photoTrigger = false; // 処理済みリセット
            }
            if (ctrl.recordToggle) {
                storage.toggleRecording(); // 録画を要求
                ctrl.recordToggle = false;
            }

            // --- コマンド送信 (優先順位の高い順にどれか1つだけ送る) ---
            // 優先順位：緊急停止 > 着陸 > 離陸 > flip > RC
            if (ctrl.emergencyTrigger) {
                sender.send("emergency");
                ctrl.emergencyTrigger = false; // 処理済みリセット
            } 
            else if (ctrl.landTrigger) {
                sender.send("land");
                ctrl.landTrigger = false;
            } 
            else if (ctrl.takeOffTrigger) {
                sender.send("takeoff");
                ctrl.takeOffTrigger = false;
            } 
            else if (ctrl.flipLTrigger) {
                sender.send("flip l");
                ctrl.flipLTrigger = false;
            } 
            else if (ctrl.flipRTrigger) {
                sender.send("flip r");
                ctrl.flipRTrigger = false;
            } 
            else {
                // 特別なボタンが何も押されていない時だけ、スティック移動(RC)を送る
                mapper.map(ctrl);
                sender.send(TelloCommand::makeRc(ctrl));
            }

            last_rc_time = now;
        }

        // 2. 【WiFi強度取得】 1000ms (1秒) おきに固定
        auto wifi_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_wifi_time).count();
        if (wifi_elapsed >= 1000) {
            std::string res = sender.sendAndWaitResponse("wifi?", 30);
            if (!res.empty() && res.find("error") == std::string::npos) {
                g_telemetry.rssi.store(std::atoi(res.c_str()));
            }
            last_wifi_time = now;
        }

        // 3. 【デコード・描画】 ここは可能な限り速く（FPSに依存）
        VideoPacket pkt;
        while (h264Analyzer.pop(pkt)) {
            // 録画マネージャにパケットを投げる(録画中かどうかはマネージャが判断する)
            storage.pushVideoPacket(pkt.data);

            // decode実行
            decoder.decode(pkt, 
                            vita2d_texture_get_datap(frame_texture), 
                            vita2d_texture_get_stride(frame_texture)
            );
        } // end while

        // 4. 【FPS計算】
        frame_count++;
        auto fps_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_time).count();
        if (fps_elapsed >= 1000) {
            current_fps = frame_count;
            frame_count = 0;
            last_fps_time = now;
        }


        // 描画処理
        vita2d_start_drawing();
        vita2d_clear_screen();

        // 描画：960x720テクスチャを960x544画面にフィットさせる
        vita2d_draw_texture_scale(
            frame_texture, 
            0, 0, 
            (float)PSVITA_SCREEN_WIDTH / TELLO_VIDEO_WIDTH,
            (float)PSVITA_SCREEN_HEIGHT / TELLO_VIDEO_HEIGHT
        );

        // 上部ステータスバーの背景
        vita2d_draw_rectangle(0, 0, 960, 40, RGBA8(0, 0, 0, 100));

        
        // --- 速度Gain ---
        if (display_gain_timer > 0) {
            // 画面中央付近に現在のGainを表示
            vita2d_font_draw_textf(font, 20, 500, RGBA8(255, 255, 255, 150), 24, "Gain: %d%%", (int)(mapper.getGain() * 100));
            display_gain_timer -= 16.6f; // およそ1フレーム分引く
        }

        // --- バッテリー残量 ---
        int bat = g_telemetry.battery.load();
        unsigned int batColor = (bat < 20) ? RGBA8(255, 0, 0, 150) : (bat < 50) ? RGBA8(255, 255, 0, 150) : RGBA8(0, 255, 0, 150);
        vita2d_font_draw_textf(font, 20, 30, batColor, 24, "BAT: %d%%", bat);

        // --- 最高温度 (temph) ---
        int temp = g_telemetry.temph.load();
        unsigned int tempColor = (temp > 80) ? RGBA8(255, 0, 0, 150) : (temp > 75) ? RGBA8(255, 255, 0, 150) : RGBA8(255, 255, 255, 150);
        vita2d_font_draw_textf(font, 180, 30, tempColor, 24, "TEMP: %dC", temp);

        // --- 電波強度 (取れている場合のみ表示) ---
        int rssi = g_telemetry.rssi.load();
        if (rssi > 0) {
            unsigned int rssiColor = (rssi < 40) ? RGBA8(255, 0, 0, 150) : RGBA8(0, 255, 0, 150);
            vita2d_font_draw_textf(font, 340, 30, rssiColor, 24, "WIFI: %d", rssi);
        }

        // --- 高度 (HとTOFの両方を表示すると便利) ---
        // int height = g_telemetry.h.load(); // 気圧高度
        int tof = g_telemetry.tof.load(); // 地面との距離
        unsigned int tofColor = (tof > 300) ? RGBA8(255, 0, 0, 150) : RGBA8(255, 255, 255, 150);
        // vita2d_font_draw_textf(font, 500, 30, RGBA8(255, 255, 255, 150), 24, "H: %dcm", height);
        vita2d_font_draw_textf(font, 500, 30, tofColor, 24, "TOF: %dcm", tof);

        // --- FPS ---
        vita2d_font_draw_textf(font, 660, 30, RGBA8(255, 255, 255, 150), 24, "FPS: %d", current_fps);

        // 録画中表示
        //  録画準備中（ファイルは開いたが、まだ映像データが書き込めていない）
        if (storage.isPreparing()) {
            vita2d_draw_rectangle(800, 10, 20, 20, RGBA8(255, 255, 0, 150));
            vita2d_font_draw_text(font, 830, 30, RGBA8(255, 255, 0, 150), 18, "REC PREPARE");
        }
        //  録画中（SPSを見つけ、ファイル書き込みが正常に行われている）
        else if (storage.isRecording()) {
            vita2d_draw_fill_circle(820, 22, 8, RGBA8(255, 0, 0, 150)); 
            vita2d_font_draw_text(font, 845, 30, RGBA8(255, 0, 0, 150), 24, "REC");
        }

        // 撮影フラッシュエフェクト (フェードアウト実装)
        if (flash_alpha > 0) {
            vita2d_draw_rectangle(0, 0, 960, 544, RGBA8(255, 255, 255, flash_alpha));
            // 毎フレーム5ずつ不透明度を下げる 
            flash_alpha -= 5; 
            if (flash_alpha < 0) flash_alpha = 0;
        }


        vita2d_end_drawing();
        vita2d_swap_buffers();

        // 画像は100Hz超えない想定で100Hz周期
        // TODO: 処理時間も計測して100Hzになるようにsleep
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}
