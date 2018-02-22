// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <cinttypes>
#include "common/common_types.h"
#include "common/logging/log.h"
#include "core/hle/ipc.h"
#include "core/hle/ipc_helpers.h"
#include "core/hle/result.h"
#include "core/hle/service/mcu/cam.h"
#include "core/hle/service/mcu/cdc.h"
#include "core/hle/service/mcu/gpu.h"
#include "core/hle/service/mcu/hid.h"
#include "core/hle/service/mcu/hwc.h"
#include "core/hle/service/mcu/mcu.h"
#include "core/hle/service/mcu/nwm.h"
#include "core/hle/service/mcu/pls.h"
#include "core/hle/service/mcu/rtc.h"
#include "core/hle/service/mcu/snd.h"
#include "core/hle/shared_page.h"

namespace Service {
namespace MCU {

void Module::Interface::GetBatteryLevel(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0005, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(2, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(0x64);
    LOG_WARNING(Service_MCU, "(STUBBED) called");
}

void Module::Interface::Set3DLEDState(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x0009, 1, 0);
    u8 state = rp.Pop<u8>();
    SharedPage::shared_page.ledstate_3d = state;
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
}

void Module::Interface::GetSoundVolume(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x000B00, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(0x3F);
    LOG_WARNING(Service_MCU, "(STUBBED) called");
}

void Module::Interface::GetSoundVolume2(Kernel::HLERequestContext& ctx) {
    IPC::RequestParser rp(ctx, 0x000E00, 0, 0);
    IPC::RequestBuilder rb = rp.MakeBuilder(1, 0);
    rb.Push(RESULT_SUCCESS);
    rb.Push<u8>(0x3F);
    LOG_WARNING(Service_MCU, "(STUBBED) called");
}

Module::Interface::Interface(std::shared_ptr<Module> mcu, const char* name, u32 max_session)
    : ServiceFramework(name, max_session), mcu(std::move(mcu)) {}

void InstallInterfaces(SM::ServiceManager& service_manager) {
    auto mcu = std::make_shared<Module>();
    std::make_shared<NWM>(mcu)->InstallAsService(service_manager);
}

} // namespace MCU
} // namespace Service
