// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <future>
#include <string>
#include <vector>
#include "common/common_types.h"

namespace NetplayAnnounce {

using MacAddress = std::array<u8, 6>;

struct Room {
    struct Member {
        std::string name;
        MacAddress mac_address;
        std::string game_name;
        u64 game_id;
    };
    std::string name;
    std::string GUID;
    std::string ip;
    u16 port;
    u32 max_player;
    u32 net_version;
    bool has_password;

    std::vector<Member> members;
};
using RoomList = std::vector<Room>;

/**
 * A NetplayAnnounce interface class. A backend to submit/get to/from a web service should implement
 * this interface.
 */
class Backend : NonCopyable {
public:
    virtual ~Backend() = default;
    virtual void SetRoomInformation(const std::string& guid, const std::string& name,
                                    const u16 port, const u32 max_player, const u32 net_version,
                                    const bool has_password) = 0;
    virtual void AddPlayer(const std::string& nickname, const MacAddress& mac_address,
                           const u64 game_id, const std::string& game_name) = 0;
    virtual void Announce() = 0;
    virtual void ClearPlayers() = 0;
    virtual std::future<RoomList> GetRoomList() = 0;
    virtual void Delete() = 0;
};

/**
 * Empty implementation of NetplayAnnounce interface that drops all data. Used when a functional
 * backend implementation is not available.
 */
class NullBackend : public Backend {
public:
    ~NullBackend() = default;
    void SetRoomInformation(const std::string& /*guid*/, const std::string& /*name*/,
                            const u16 /*port*/, const u32 /*max_player*/, const u32 /*net_version*/,
                            const bool /*has_password*/) override {}
    void AddPlayer(const std::string& /*nickname*/, const MacAddress& /*mac_address*/,
                   const u64 /*game_id*/, const std::string& /*game_name*/) override {}
    void Announce() override {}
    void ClearPlayers() override {}
    std::future<RoomList> GetRoomList() override {
        auto EmptyRoomList = []() -> RoomList { return RoomList{}; };
        return std::async(EmptyRoomList);
    }

    void Delete() override {}
};

} // namespace NetplayAnnounce
