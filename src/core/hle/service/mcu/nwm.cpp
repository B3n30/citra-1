// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/hle/service/mcu/nwm.h"

namespace Service {
namespace MCU {

NWM::NWM(std::shared_ptr<Module> mcu) : Module::Interface(std::move(mcu), "mcu::NWM", 1) {
    static const FunctionInfo functions[] = {
        {0x00010040, nullptr, "SetWirelessLedState"},
        {0x00030040, nullptr, "Sets GPIO 0x20 high/low?"},
        {0x00050040, nullptr, "SetEnableWifiGpio"},
        {0x00080000, nullptr, "GetWirelessDisabledFlag"},
    };
    RegisterHandlers(functions);
}


} // namespace MCU
} // namespace Service
