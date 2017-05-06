// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/log.h"
#include "net_core/local.h"

namespace NetCore {
namespace Local {

bool Init() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return true;
}

void Shutdown() {
    LOG_DEBUG(Network, "(STUBBED) called");
}

MemberState GetMemberState() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return MemberState::Idle;
}

RoomState GetRoomState() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return RoomState::Closed;
}

bool IsConnected() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return false;
}

MemberList GetMemberInformation() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return {};
}

RoomInformation GetRoomInformation() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return {};
}

std::deque<ChatEntry> PopChatEntries() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return {};
}

std::deque<WifiPacket> PopWifiPackets(WifiPacket::PacketType type, const MacAddress& sender) {
    LOG_DEBUG(Network, "(STUBBED) called");
    return {};
}

void SendChatMessage(const std::string& message) {
    LOG_DEBUG(Network, "(STUBBED) called");
}

void SendWifiPacket(const WifiPacket& packet) {
    LOG_DEBUG(Network, "(STUBBED) called");
}

std::string GetStatistics() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return "";
}

int GetPing() {
    LOG_DEBUG(Network, "(STUBBED) called");
    return 0;
}

void Join(const std::string& nickname, const std::string& server, const uint16_t serverPort, const uint16_t clientPort) {
    LOG_DEBUG(Network, "(STUBBED) called");
}

void Leave() {
    LOG_DEBUG(Network, "(STUBBED) called");
}

void Create(const std::string& name, const std::string& server, uint16_t server_port) {
    LOG_DEBUG(Network, "(STUBBED) called");
}

void Destroy() {
    LOG_DEBUG(Network, "(STUBBED) called");
}

} // namespace
} // namespace
