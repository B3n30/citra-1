// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <future>
#include <json.hpp>
#include "core/settings.h"
#include "web_service/netplay_json.h"
#include "web_service/web_backend.h"

namespace NetplayAnnounce {

void to_json(nlohmann::json& json, const Room::Member& member) {
    json["name"] = member.name;
    json["gameName"] = member.game_name;
    json["gameVersion"] = member.game_version;
    json["gameId"] = member.game_id;
}

void from_json(const nlohmann::json& json, Room::Member& member) {
    member.name = json.at("name").get<std::string>();
    member.game_name = json.at("gameName").get<std::string>();
    member.game_id = json.at("gameId").get<u64>();
    member.game_version = json.at("gameVersion").get<u16>();
}

void to_json(nlohmann::json& json, const Room& room) {
    json["id"] = room.GUID;
    json["port"] = room.port;
    json["name"] = room.name;
    json["maxPlayers"] = room.max_player;
    json["netVersion"] = room.net_version;
    if (room.members.size() > 0) {
        nlohmann::json member_json = room.members;
        json["players"] = member_json;
    }
}

void from_json(const nlohmann::json& json, Room& room) {
    room.ip = json.at("ip").get<std::string>();
    room.name = json.at("name").get<std::string>();
    room.port = json.at("port").get<u16>();
    room.max_player = json.at("maxPlayers").get<u32>();
    room.net_version = json.at("netVersion").get<u32>();
    try {
        room.members = json.at("players").get<std::vector<Room::Member>>();
    } catch (const std::out_of_range&) {
        LOG_DEBUG(WebService, "no member in room");
    }
}

} // namespace NetplayAnnounce

namespace WebService {

void NetplayJson::SetRoomInformation(const std::string& guid, const std::string& name,
                                     const u16 port, const u32 max_player, const u32 net_version) {
    room.name = name;
    room.GUID = guid;
    room.port = port;
    room.max_player = max_player;
    room.net_version = net_version;
}
void NetplayJson::AddPlayer(const std::string& nickname,
                            const NetplayAnnounce::MacAddress& mac_address, const u64 game_id,
                            const std::string& game_name, const u32 game_version) {
    NetplayAnnounce::Room::Member member;
    member.name = nickname;
    member.mac_address = mac_address;
    member.game_id = game_id;
    member.game_name = game_name;
    member.game_version = game_version;
    room.members.push_back(member);
}

void NetplayJson::Announce() {
    nlohmann::json json = room;
    PostJson(Settings::values.announce_netplay_endpoint_url, json.dump());
}

void NetplayJson::ClearPlayers() {
    room.members.clear();
}

std::future<NetplayAnnounce::RoomList> NetplayJson::GetRoomList() {
    std::future<std::string> reply = GetJson(Settings::values.announce_netplay_endpoint_url);
    auto DeSerialize = [&]() -> NetplayAnnounce::RoomList {
        ;
        nlohmann::json json = reply.get();
        NetplayAnnounce::RoomList room_list = json;
        return room_list;
    };
    return std::async(DeSerialize);
}

void NetplayJson::Delete() {
    nlohmann::json json;
    json["GUID"] = room.GUID;
    DeleteJson(Settings::values.announce_netplay_endpoint_url, json.dump());
}

} // namespace WebService
