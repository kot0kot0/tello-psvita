# TelloPSVita
# [tello-psvita](https://github.com/kot0kot0/tello-psvita)

PlayStation Vitaをコントローラーとして使用し、DJI Telloを操縦・録画・撮影するための自作アプリケーション。Vitaのハードウェアデコーダーを活用し、低遅延を実現。

## ⚠️ 免責事項

- 本アプリは自作ソフト（Homebrew）です。使用によるドローンの紛失、衝突、破損、またはその他の損害について、開発者は一切の責任を負いません。
- 屋外で使用する場合は、周囲の安全を十分に確認し、電波法および航空法を遵守してください。


## 🚀 主な機能

- **リアルタイム映像伝送**: TelloからのH.264ストリームをVitaのハードウェアデコーダー（SceVideoDec）で低遅延表示。
- **フルコントロール**: スティックによる直感的なRC操作、ボタンによる速度調整、フリップ、緊急停止、撮影、録画などに対応。
- **メディア保存**: 映像の録画(.h264)および静止画（.jpeg）を`ux0:data/TelloPSVita/'に保存。
- **OSDテレメトリ**: バッテリー残量、温度、Wi-Fi強度、高度（TOF）、デコードFPSをリアルタイム表示。
- **ゲイン調整**: 十字キーで移動スピードを10%刻みで調整可能（画面上に通知表示）。

## 🕹 コントロール（操作方法）

| 操作 | 動作 | 備考 |
| :--- | :--- | :--- |
| **左スティック** | 上昇・下降 / 旋回（Yaw） | |
| **右スティック** | 前進・後進 / 左右スライド | |
| **△ (長押し 1.5s)** | 離陸 (Takeoff) |  |
| **× (長押し 1.5s)** | 着陸 (Land) |  |
| **□ (長押し 1.5s)** | 左フリップ (Flip Left) | |
| **○ (長押し 1.5s)** | 右フリップ (Flip Right) | |
| **Rボタン** | 静止画撮影 | 画面フラッシュ演出付 |
| **Lボタン** | 録画開始/停止 | 画面内にREC表示 |
| **十字キー 上/下** | スピードゲイン調整 | 10%刻み(0.2〜1.0) |
| **START + SELECT** | **緊急停止 (Emergency)** | モーターが即座に停止 |

## 画面構成
...

## 📁 フォルダ構成と役割

プロジェクトは機能ごとにモジュール化されています。

### 📂 `src/app` (エントリーポイント)
- **`VitaMain.cpp`**: プログラムの基点。初期化、メインループ、UI描画、コマンド送信の優先順位管理を担当。

### 📂 `src/core/` (映像処理・データ管理)
- **`VitaAvcDecoder.hpp/cpp`**: Vitaのハードウェアデコーダー（`SceVideoDec`）の制御。
- **`H264AnalyzeWorker.hpp/cpp`**: 受信データからNALユニット（SPS/PPS/IDR）を解析・抽出。
- **`MediaStorageManager.hpp/cpp`**: `ux0:data/` への動画書き込みおよび静止画保存の管理。
- **`RingBuffer.hpp`**: ネットワークと解析スレッド間のスレッドセーフなデータ受け渡し。
- **`TelemetryState.hpp`**: 機体の状態（バッテリー、温度等）を保持する共有構造体。
- **`TelloRcMapper.hpp`**: スティック入力をTello用数値へマッピングし、ゲイン計算を行う。

### 📂 `src/net/` (ネットワーク通信)
- **`UdpReceiver.hpp/cpp`**: 映像ストリーム受信用のUDPリスナー。
- **`UdpCommandSender.hpp/cpp`**: Telloへのコマンド送信およびレスポンス待ち受け。
- **`TelemetryReceiver.hpp/cpp`**: Telloの状態パケットをパースし、`TelemetryState`を更新。
- **`TelloCommand.hpp/cpp`**: Tello用テキストコマンドの生成ユーティリティ。
- **`UdpLogger.hpp/cpp`**: psvitaでログ表示は難しいためログ情報を別の機器へUDP送信する。

### 📂 `src/platform/vita/` (Vitaハードウェア依存)
- **`VitaInput.hpp/cpp`**: ボタンの単押し、長押し、デッドゾーン処理等、Vitaの入力管理。

## 🛠 開発環境 / 依存関係

- **Compiler**: [VitaSDK](https://vitasdk.org/)
- **Libraries**:
    - `vita2d`: 描画エンジン
    - `libjpeg-turbo`: 写真保存用
    - `SceVideoDec`: ハードウェアデコード
    - `SceNet`: UDP通信
    - ...  

## 環境構築手順
...  

## 実行手順
1. Tello Eduの電源を入れて、正面のLEDが赤青黄色で点滅することを確認
2. PSVitaの`設定`アプリから`ネットワーク`>`Wi-Fi設定`に選択して、TELLO-***というSSIDを選択して接続する  
   (Tello Eduの正面のLEDが黄色点滅に変わる)
3. 環境構築にてinstallした本アプリ`Tello Controller`を選択して、ゲームを開始する
   (Tello Eduと通信に成功するとTello Eduの正面のLEDが緑色点滅に変わる)
...

## 撮影/録画データの取り出し手順
...

