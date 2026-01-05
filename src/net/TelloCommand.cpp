#include "TelloCommand.hpp"
#include <cstdio>

std::string TelloCommand::makeRc(const ControlState& s) {
    char buf[64];
    std::snprintf(
        buf, sizeof(buf),
        "rc %d %d %d %d",
        s.lr, s.fb, s.ud, s.yaw
    );
    return std::string(buf);
}
