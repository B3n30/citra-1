// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <vector>
#include "common/common_types.h"
#include "core/cheats/cheat_base.h"

namespace Core {
class System;
}

namespace CoreTiming {
struct EventType;
}

namespace Cheats {

class CheatEngine {
public:
    explicit CheatEngine(Core::System& system);
    ~CheatEngine();
    std::vector<std::shared_ptr<CheatBase>> GetCheats();

private:
    void LoadCheatFile();
    void RunCallback(u64 /*userdata*/, int cycles_late);
    std::vector<std::shared_ptr<CheatBase>> cheats_list;
    CoreTiming::EventType* event;
    Core::System& system;
};
} // namespace Cheats
