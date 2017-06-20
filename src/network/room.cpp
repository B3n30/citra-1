// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "network/room.h"

namespace Network {

/// Maximum number of concurrent connections allowed to this room.
static const uint32_t MaxConcurrentConnections = 10;

void Room::Create(const std::string& name, const std::string& server_address, uint16_t server_port) {
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    enet_address_set_host(&address, server_address.c_str());
    address.port = server_port;

    server = enet_host_create(&address, MaxConcurrentConnections, numChannels, 0, 0);
    // TODO(B3N30): Allow specifying the maximum number of concurrent connections.
    state = State::Open;

    room_information.name = name;
    room_information.member_slots = MaxConcurrentConnections;

    // TODO(B3N30): Start the receiving thread
}

void Room::Destroy() {
    state = State::Closed;
    // TODO(B3n30): Join the receiving thread

    if (server) {
        enet_host_destroy(server);
    }
    room_information = {};
    server = nullptr;
}

} // namespace
