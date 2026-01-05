#!/bin/bash
set -e # エラーが発生したら即停止

# 1. ビルドディレクトリ作成
mkdir -p build
cd build

# 2. 古いキャッシュをクリア（プロの現場では推奨）
rm -f CMakeCache.txt

# 3. ターゲットに応じたビルド（例：Vita向け）

# USE_NET_LOGGERはPSVITAのときしかONにできない(netdebug.hをincludeする必要があるため)
# TARGET_PLATFORM = PSVITA or PC
#cmake -DUSE_NET_LOGGER=OFF \
#      -DTARGET_PLATFORM=PC .. 
      #-DCMAKE_BUILD_TYPE=Debug \

cmake -DTARGET_PLATFORM=PC \
      -DUSE_NET_LOGGER=ON \
      -DLOGGER_IP="192.168.10.120" \
      -DLOGGER_PORT="12345" ..

make -j$(nproc) # 並列ビルドで高速化