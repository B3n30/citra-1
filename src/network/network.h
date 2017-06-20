// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include "network/room_member.h"

namespace Network {

/// Initializes and registers the network device, the room, and the room member.
bool Init();

/// Returns a pointer to the room handle
Room* getRoom();

/// Returns a pointer to the room member handle
RoomMember* getRoomMember();

/// Unregisters the network device, the room, and the room member and shut them down.
void Shutdown();

} // namespace
