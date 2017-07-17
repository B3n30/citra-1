// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <cpr/cpr.h>
#include <future>
#include <list>
#include <json.hpp>
#include "common/common_types.h"

namespace WebService {

class RoomJson {
public:
    struct Member {
        std::string name{""};
        std::string game_name{""};
        u64 game_id{};
        u16 game_version{0};
    };
    using MemberList = std::list<Member>;

    struct Room {
        std::string id{""};
        std::string name{""};
        std::string ip{""};
        u16 port{0};
        u32 net_version{0};
        u32 slots{0};
        MemberList members{};
    };
    using RoomList = std::list<Room>;

    void SetRoomInfo(std::string GUID, const std::string& name, const u16 port, const u32 slots, u32 network_version);

    void SetMembers(const MemberList& members);

    void Announce();

    void SendDelete();

    const RoomList& Get();
private:
    Room room;
    std::future<cpr::Response> response;
    RoomList room_list;

};

} // namespace WebService
