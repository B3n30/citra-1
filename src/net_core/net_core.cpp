// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "net_core/local.h"
#include "net_core/net_core.h"

namespace NetCore {

bool Init() {
    return NetCore::Local::Init();
}

void Shutdown() {
    NetCore::Local::Shutdown();
}

} // namespace
