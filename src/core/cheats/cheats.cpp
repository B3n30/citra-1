// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <functional>
#include <fmt/format.h>
#include "core/cheats/cheats.h"
#include "core/cheats/gateway_cheat.h"
#include "core/core.h"
#include "core/core_timing.h"
#include "core/hle/kernel/process.h"

namespace Cheats {

static constexpr u64 run_interval_ticks = BASE_CLOCK_RATE_ARM11 / 60;

CheatEngine::CheatEngine(Core::System& system_) : system(system_) {
    LoadCheatFile();
    event = CoreTiming::RegisterEvent(
        "CheatCore::run_event",
        std::bind(&CheatEngine::RunCallback, this, std::placeholders::_1, std::placeholders::_2));
    CoreTiming::ScheduleEvent(run_interval_ticks, event);
}

CheatEngine::~CheatEngine() {
    CoreTiming::UnscheduleEvent(event, 0);
}

std::vector<std::shared_ptr<CheatBase>> CheatEngine::GetCheats() {
    return cheats_list;
}

void CheatEngine::LoadCheatFile() {
    std::string filepath =
        fmt::format("{}{:016X}.txt", FileUtil::GetUserPath(FileUtil::UserPath::CheatsDir),
                    system.Kernel().GetCurrentProcess()->codeset->program_id);

    FileUtil::CreateFullPath(filepath); // Create path if not already created

    auto gateway_cheats = GatewayCheat::LoadFile(filepath);
    cheats_list.insert(cheats_list.end(), gateway_cheats.begin(), gateway_cheats.end());
}

void CheatEngine::RunCallback(u64 /*userdata*/, int cycles_late) {
    for (auto& cheat : cheats_list) {
        if (cheat->GetEnabled()) {
            cheat->Execute(system);
        }
    }
    CoreTiming::ScheduleEvent(run_interval_ticks - cycles_late, event);
}

} // namespace Cheats
