cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()


project(tello_psvita LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Vita SDK 前提
if(NOT DEFINED ENV{VITASDK})
    message(FATAL_ERROR "VITASDK is not set")
endif()

set(VITASDK $ENV{VITASDK})
include(${VITASDK}/share/vita.cmake)


# ----------------------------
# ソース
# ----------------------------
set(SOURCES
    src/app/VitaMain.cpp

    src/core/RingBuffer.cpp
    src/core/H264AnnexBAssembler.cpp
    src/core/H264AnalyzeWorker.cpp
    src/core/VitaAvcDecoder.cpp
    src/core/MediaStorageManager.cpp

    src/net/UdpReceiver.cpp
    src/net/TelemetryReceiver.cpp
    src/net/UdpCommandSender.cpp
    src/net/TelloCommand.cpp
    src/net/UdpLogger.cpp

    src/platform/vita/VitaInput.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        src
)

# ----------------------------
# Vita 用リンク
# ----------------------------
target_link_libraries(${PROJECT_NAME}
  /usr/local/vitasdk/arm-vita-eabi/lib/libSceAvcodec_stub.a
  /usr/local/vitasdk/arm-vita-eabi-ffmpeg/lib/libavformat.a 
  /usr/local/vitasdk/arm-vita-eabi-ffmpeg/lib/libavcodec.a 
  /usr/local/vitasdk/arm-vita-eabi/lib/libSceVideodec_stub.a
  /usr/local/vitasdk/arm-vita-eabi/lib/libSceCodecEngine_stub.a 
  /usr/local/vitasdk/arm-vita-eabi/lib/libSceAudiodec_stub.a
  /usr/local/vitasdk/arm-vita-eabi-ffmpeg/lib/libavutil.a 
  /usr/local/vitasdk/arm-vita-eabi-ffmpeg/lib/libswscale.a 
  /usr/local/vitasdk/arm-vita-eabi-ffmpeg/lib/libswresample.a
  /usr/local/vitasdk/arm-vita-eabi/lib/libvita2d.a
  #mp3lame
  #lzma
  #/usr/local/vitasdk/arm-vita-eabi/lib/libm.a
  #/usr/local/vitasdk/arm-vita-eabi/lib/libz.a
  #SDL2::SDL2
  #debugnet # udp logger
  

    vita2d
    freetype
    png
    jpeg
    z
    m
    debugnet

    SceNet_stub
    SceNetCtl_stub
    SceSysmodule_stub
    SceDisplay_stub
    SceGxm_stub
    SceLibKernel_stub
    SceCtrl_stub

    SceAppMgr_stub
    SceCommonDialog_stub
    bz2

    -Wl,--whole-archive pthread -Wl,--no-whole-archive
    stdc++
)

add_compile_definitions(TARGET_VITA)
add_compile_options(-O3 -mfloat-abi=hard -ffast-math)

# ----------------------------
# Vita 出力
# ----------------------------
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})
vita_create_vpk(${PROJECT_NAME}.vpk
    VSDK00142
    ${PROJECT_NAME}.self
    VERSION 01.00
    NAME "Tello Controller"
)
