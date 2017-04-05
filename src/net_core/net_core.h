// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <tunnler/room.h>
#include <tunnler/room_member.h>
#include <tunnler/tunnler.h>

namespace NetCore {

/// Initialise Net Core
bool Init();

Room* getRoom();

RoomMember* getRoomMember();

/// Shutdown Net Core
void Shutdown();

} // namespace
