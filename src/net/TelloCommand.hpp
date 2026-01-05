#pragma once
#include <string>
#include "core/ControlState.hpp"

class TelloCommand {
public:
    static std::string makeRc(const ControlState& s);
};
