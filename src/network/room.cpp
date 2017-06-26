// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "network/packet.h"
#include "network/room.h"

namespace Network {

/// Maximum number of concurrent connections allowed to this room.
static const uint32_t MaxConcurrentConnections = 10;

// This MAC address is used to generate a 'Nintendo' like Mac address.
const MacAddress NintendoOUI = { 0x00, 0x1F, 0x32, 0x00, 0x00, 0x00 };

void Room::Create(const std::string& name, const std::string& server_address,
                  uint16_t server_port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    enet_address_set_host(&address, server_address.c_str());
    address.port = server_port;

    server = enet_host_create(&address, MaxConcurrentConnections, NumChannels, 0, 0);
    // TODO(B3N30): Allow specifying the maximum number of concurrent connections.
    state = State::Open;

    room_information.name = name;
    room_information.member_slots = MaxConcurrentConnections;

    room_thread = std::make_unique<std::thread>(&Room::ServerLoop, this);
}

void Room::Destroy() {
    state = State::Closed;
    room_thread->join();

    if (server) {
        enet_host_destroy(server);
    }
    room_information = {};
    server = nullptr;
}

void Room::ServerLoop() {
    while (state != State::Closed) {
        ENetEvent event;
        while (enet_host_service(server, &event, 1000) > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                switch (event.packet->data[0]) {
                case IdWifiPacket:
                    // Received a wifi packet, broadcast it to everyone else except the sender.
                    // TODO(B3N30): Maybe change this to a loop over `members`, since we only want to
                    // send this data to the people who have actually joined the room.
                    enet_host_broadcast(server, 0, event.packet);
                    enet_host_flush(server);
                    break;
                case IdChatMessage:
                    HandleChatPacket(&event);
                    break;
                case IdJoinRequest:
                    HandleJoinRequest(&event);
                    break;
                }
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                HandleClientDisconnection(event.peer);
                break;
            }
        }
    }
}

void Room::HandleJoinRequest(const ENetEvent* event) {
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);
    packet.IgnoreBytes(sizeof(MessageID));
    std::string nickname;
    packet >> nickname;

    MacAddress preferred_mac;
    packet >> preferred_mac;

    if (!IsValidNickname(nickname)) {
        SendNameCollision(event->peer);
        return;
    }

    if (preferred_mac != NoPreferredMac) {
        // Verify if the preferred mac is available
        if (!IsValidMacAddress(preferred_mac)) {
            SendMacCollision(event->peer);
            return;
        }
    } else {
        // Assign a MAC address of this client automatically
        preferred_mac = GenerateMacAddress();
    }

    // At this point the client is ready to be added to the room.
    Member member{};
    member.mac_address = preferred_mac;
    member.nickname = nickname;
    member.network_address = event->peer->address;

    members.push_back(member);

    // Notify everyone that the room information has changed.
    BroadcastRoomInformation();

    SendJoinSuccess(event->peer);
}

bool Room::IsValidNickname(const std::string& nickname) const {
    // A nickname is valid if it is not already taken by anybody else in the room.
    // TODO(B3N30): Check for empty names, spaces, etc.
    for (const Member& member : members) {
        if (member.nickname == nickname) {
            return false;
        }
    }
    return true;
}

bool Room::IsValidMacAddress(const MacAddress& address) const {
    // A MAC address is valid if it is not already taken by anybody else in the room.
    for (const Member& member : members) {
        if (member.mac_address == address) {
            return false;
        }
    }
    return true;
}

void Room::SendNameCollision(ENetPeer* client) {
    Packet packet;
    packet << static_cast<MessageID>(IdNameCollision);

    ENetPacket* enet_packet = enet_packet_create(packet.GetData(), packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
    enet_peer_disconnect(client, 0);
}

void Room::SendMacCollision(ENetPeer* client) {
    Packet packet;
    packet << static_cast<MessageID>(IdMacCollision);

    ENetPacket* enet_packet = enet_packet_create(packet.GetData(), packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
    enet_peer_disconnect(client, 0);
}

void Room::SendJoinSuccess(ENetPeer* client) {
    Packet packet;
    packet << static_cast<MessageID>(IdJoinSuccess);
    packet << client->address.host;
    ENetPacket* enet_packet = enet_packet_create(packet.GetData(), packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(client, 0, enet_packet);
    enet_host_flush(server);
}

void Room::BroadcastRoomInformation() {
    Packet packet;
    packet << static_cast<MessageID>(IdRoomInformation);
    packet << room_information.name;
    packet << room_information.member_slots;

    packet << static_cast<uint32_t>(members.size());
    for (const auto& member: members) {
        packet << member.nickname;
        packet << member.mac_address;
        packet << member.game_name;
    }

    ENetPacket* enet_packet = enet_packet_create(packet.GetData(), packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, enet_packet);
    enet_host_flush(server);
}

void Room::HandleChatPacket(const ENetEvent* event) {
    Packet in_packet;
    in_packet.Append(event->packet->data, event->packet->dataLength);

    in_packet.IgnoreBytes(sizeof(MessageID));
    std::string message;
    in_packet >> message;
    auto CompareNetworkAddress = [&](const Member member) -> bool {
        return member.network_address.host == event->peer->address.host;
    };
    const auto sending_member = std::find_if(members.begin(), members.end(), CompareNetworkAddress);
    ASSERT_MSG(sending_member != members.end(), "Received a chat message from a unknown sender");

    Packet out_packet;
    out_packet << static_cast<MessageID>(IdChatMessage);
    out_packet << sending_member->nickname;
    out_packet << message;

    ENetPacket* enet_packet = enet_packet_create(out_packet.GetData(), out_packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_host_broadcast(server, 0, enet_packet);
    enet_host_flush(server);
}

void Room::HandleClientDisconnection(ENetPeer*  client) {
    // Remove the client from the members list.
    members.erase(std::remove_if(members.begin(), members.end(), [&](const Member& member) {
        return member.network_address.host == client->address.host;
    }), members.end());

    // Announce the change to all other clients.
    BroadcastRoomInformation();
}

MacAddress Room::GenerateMacAddress() {
    MacAddress result_mac = NintendoOUI;
    std::uniform_int_distribution<> dis(0x00, 0xFF); //Random byte between 0 and 0xFF
    do {
        for (int i = 3; i < result_mac.size(); ++i) {
            result_mac[i] = dis(random_gen);
        }
    } while (!IsValidMacAddress(result_mac));
    return result_mac;
}

} // namespace
