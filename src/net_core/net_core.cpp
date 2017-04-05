// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <memory>
#include "common/logging/log.h"
#include "net_core/net_core.h"

namespace NetCore {

std::unique_ptr<RoomMember> g_room_member; ///< RoomMember (Client) for network games
std::unique_ptr<Room> g_room; ///< Room (Server) for network games

bool Init() {
    g_room = std::make_unique<Room>();
    g_room_member = std::make_unique<RoomMember>();
    LOG_DEBUG(Network, "initialized OK");
    return true;
}

Room* getRoom() {
    return g_room.get();
}

RoomMember* getRoomMember() {
    return g_room_member.get();
}

void Shutdown() {
    if (g_room.get()->GetState() == Room::State::Open)
        g_room.get()->Destroy();
    g_room.reset();
    if (g_room_member.get()->IsConnected())
        g_room_member.get()->Leave();
    g_room_member.reset();
    LOG_DEBUG(Network, "shutdown OK");
}

} // namespace
