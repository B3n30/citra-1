// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <string>
#include "common/netplay_announce.h"

namespace WebService {

/**
 * Implementation of NetplayAnnounce::Backend that (de)serializes room information into/from JSON,
 * and submits/gets it to/from the Citra web service
 */
class NetplayJson : public NetplayAnnounce::Backend {
public:
    ~NetplayJson() = default;
    void SetRoomInformation(const std::string& guid, const std::string& name, const u16 port,
                            const u32 max_player, const u32 net_version,
                            const bool has_password) override;
    void AddPlayer(const std::string& nickname, const NetplayAnnounce::MacAddress& mac_address,
                   const u64 game_id, const std::string& game_name) override;
    void Announce() override;
    void ClearPlayers() override;
    std::future<NetplayAnnounce::RoomList> GetRoomList() override;
    void Delete() override;

private:
    NetplayAnnounce::Room room;
};

} // namespace WebService
