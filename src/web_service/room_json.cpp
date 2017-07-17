// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "core/settings.h"
#include "web_service/room_json.h"
#include "web_service/web_backend.h"

namespace WebService {

void to_json(nlohmann::json& json, const RoomJson::Room& room) {
    json["id"] = room.id;
    json["port"] = room.port;
    json["name"] = room.name;
    json["maxPlayers"] = room.slots;
    //json["netVersion"] = room.net_version;
    if (room.members.size() > 0) {
        nlohmann::json member_json = room.members;
        json["players"] = member_json;
    }
}

void from_json(const nlohmann::json& json, RoomJson::Room& room) {
    room.ip = json.at("ip").get<std::string>();
    room.name = json.at("name").get<std::string>();
    room.port = json.at("port").get<u16>();
    room.slots = json.at("maxPlayers").get<u32>();
    room.net_version = json.at("netVersion").get<u32>();
    try {
        room.members = json.at("players").get<RoomJson::MemberList>();
    } catch (const std::out_of_range&) {
        LOG_DEBUG(WebService, "no member in room");
    }
}

void to_json(nlohmann::json& json, const RoomJson::Member& member) {
    json["name"] = member.name;
    json["gameName"] = member.game_name;
    json["gameVersion"] = member.game_version;
    json["gameId"] = member.game_id;
}

void from_json(const nlohmann::json& json, RoomJson::Member& member) {
    member.name = json.at("name").get<std::string>();
    member.game_name = json.at("gameName").get<std::string>();
    member.game_id = json.at("gameId").get<u64>();
    member.game_version = json.at("gameVersion").get<u16>();
}

void RoomJson::SetRoomInfo(std::string GUID, const std::string& name, const u16 port, const u32 slots, u32 network_version) {
    room.id = GUID;
    room.name = name;
    room.port = port;
    room.slots = slots;
    room.net_version = network_version;
}

void RoomJson::SetMembers(const MemberList& members) {
    room.members = members;
}

void RoomJson::Announce() {
    nlohmann::json json = room;
    LOG_ERROR(WebService, "Json: %s", json.dump(4).c_str());
    WebService::PostJson(Settings::values.multiplayer_endpoint_url, json.dump());
}

void RoomJson::SendDelete() {
    nlohmann::json json;
    json["id"] = room.id;
    WebService::Delete(Settings::values.multiplayer_endpoint_url, json.dump());
}

const RoomJson::RoomList& RoomJson::Get() {
    response = WebService::GetJson(Settings::values.multiplayer_endpoint_url);
    if (response.valid()) {
        cpr::Response current_response = response.get();
        if (current_response.status_code >= 400) {
            LOG_ERROR(WebService, "returned error %d", current_response.status_code);
            return room_list;
        }
        nlohmann::json json = nlohmann::json::parse(current_response.text);
        room_list = json.at("rooms").get<RoomList>();
    }
    return room_list;
}

} // namespace WebService
