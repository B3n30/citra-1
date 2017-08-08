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
    json["gameId"] = member.game_id;
}

void from_json(const nlohmann::json& json, Room::Member& member) {
    member.name = json.at("name").get<std::string>();
    member.game_name = json.at("gameName").get<std::string>();
    member.game_id = json.at("gameId").get<u64>();
}

void to_json(nlohmann::json& json, const Room& room) {
    json["id"] = room.GUID;
    json["port"] = room.port;
    json["name"] = room.name;
    json["maxPlayers"] = room.max_player;
    json["netVersion"] = room.net_version;
    json["hasPassword"] = room.has_password;
    if (room.members.size() > 0) {
        nlohmann::json member_json = room.members;
        json["players"] = member_json;
    }
}

void from_json(const nlohmann::json& json, Room& room) {
    room.ip = json.at("address").get<std::string>();
    room.name = json.at("name").get<std::string>();
    room.port = json.at("port").get<u16>();
    room.max_player = json.at("maxPlayers").get<u32>();
    room.net_version = json.at("netVersion").get<u32>();
    room.has_password = json.at("hasPassword").get<bool>();
    try {
        room.members = json.at("players").get<std::vector<Room::Member>>();
    } catch (const nlohmann::detail::out_of_range&) {
    }
}

} // namespace NetplayAnnounce

namespace WebService {

void NetplayJson::SetRoomInformation(const std::string& guid, const std::string& name,
                                     const u16 port, const u32 max_player, const u32 net_version,
                                     const bool has_password) {
    room.name = name;
    room.GUID = guid;
    room.port = port;
    room.max_player = max_player;
    room.net_version = net_version;
    room.has_password = has_password;
}
void NetplayJson::AddPlayer(const std::string& nickname,
                            const NetplayAnnounce::MacAddress& mac_address, const u64 game_id,
                            const std::string& game_name) {
    NetplayAnnounce::Room::Member member;
    member.name = nickname;
    member.mac_address = mac_address;
    member.game_id = game_id;
    member.game_name = game_name;
    room.members.push_back(member);
}

void NetplayJson::Announce() {
    nlohmann::json json = room;
    PostJson(Settings::values.announce_netplay_endpoint_url, json.dump());
}

void NetplayJson::ClearPlayers() {
    room.members.clear();
}

void NetplayJson::GetRoomList(std::function<void(const NetplayAnnounce::RoomList&)> func) {
    auto DeSerialize = [func](const std::string& reply) {
        nlohmann::json json = nlohmann::json::parse(reply);
        NetplayAnnounce::RoomList room_list = json.at("rooms").get<NetplayAnnounce::RoomList>();
        func(room_list);
    };
    GetJson(Settings::values.announce_netplay_endpoint_url, DeSerialize);
}

void NetplayJson::Delete() {
    nlohmann::json json;
    json["GUID"] = room.GUID;
    DeleteJson(Settings::values.announce_netplay_endpoint_url, json.dump());
}

} // namespace WebService
