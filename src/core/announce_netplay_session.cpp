// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <array>
#include <chrono>
#include "announce_netplay_session.h"
#include "common/assert.h"
#include "network/network.h"

#ifdef ENABLE_WEB_SERVICE
#include "web_service/netplay_json.h"
#endif

namespace Core {

// Time between room is announced to web_service
static constexpr std::chrono::seconds announce_time_interval(15);

NetplayAnnounceSession::NetplayAnnounceSession() : announce(false) {
#ifdef ENABLE_WEB_SERVICE
    backend = std::make_unique<WebService::NetplayJson>();
#else
    backend = std::make_unique<NetplayAnnounce::NullBackend>();
#endif
}

void NetplayAnnounceSession::Start() {
    ASSERT_MSG(announce == false, "Announce already running");
    announce = true;
    netplay_announce_thread =
        std::make_unique<std::thread>(&NetplayAnnounceSession::AnnounceNetplayLoop, this);
}

void NetplayAnnounceSession::Stop() {
    ASSERT_MSG(announce, "No announce to stop");
    announce = false;
    // Detaching the loop, to not wait for the sleep to finish. The loop thread will finish soon.
    netplay_announce_thread->detach();
    netplay_announce_thread.reset();
    backend->Delete();
}

NetplayAnnounceSession::~NetplayAnnounceSession() {
    if (announce) {
        Stop();
    }
}

void NetplayAnnounceSession::AnnounceNetplayLoop() {
    while (announce) {
        if (std::shared_ptr<Network::Room> room = Network::GetRoom().lock()) {
            if (room->GetState() == Network::Room::State::Open) {
                Network::RoomInformation room_information = room->GetRoomInformation();
                std::array<Network::Room::Member, Network::MaxConcurrentConnections> memberlist = room->GetRoomMemberList();
                backend->SetRoomInformation(room_information.guid, room_information.name,
                                            room_information.port, room_information.member_slots,
                                            Network::network_version, room->HasPassword());
                backend->ClearPlayers();
                LOG_DEBUG(Network, "%lu", memberlist.size());
                for (const auto& member : memberlist) {
                    if (!member.nickname.empty()) {
                        backend->AddPlayer(member.nickname, member.mac_address, member.game_info.id,
                                           member.game_info.name);
                    }
                }
                backend->Announce();
            }
        }
        std::this_thread::sleep_for(announce_time_interval);
    }
}

std::future<NetplayAnnounce::RoomList> NetplayAnnounceSession::GetRoomList(std::function<void()> func) {
    return backend->GetRoomList(func);
}

} // namespace Core
