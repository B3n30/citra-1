// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <string>
#include "enet/enet.h"

namespace Network {

const uint16_t DefaultRoomPort = 1234;
const size_t NumChannels = 1; // Number of channels used for the connection

struct RoomInformation {
    std::string name;      // Name of the server
    uint32_t member_slots; // Maximum number of members in this room
};

// This is what a server [person creating a server] would use.
class Room final {
public:
    enum class State {
        Open,   // The room is open and ready to accept connections.
        Closed, // The room is not opened and can not accept connections.
    };

    Room() = default;
    ~Room() = default;

    /**
     * Gets the current state of the room.
     */
    State GetState() const {
        return state;
    };

    /**
     * Gets the room information of the room.
     */
    const RoomInformation& GetRoomInformation() const {
        return room_information;
    };

    /**
     * Creates the socket for this room. Will bind to default address if
     * server is empty string.
     */
    void Create(const std::string& name, const std::string& server = "",
                uint16_t server_port = DefaultRoomPort);

    /**
     * Destroys the socket
     */
    void Destroy();

private:
    ENetHost* server = nullptr;              ///< network interface.
    std::atomic<State> state{State::Closed}; ///< Current state of the room.

    RoomInformation room_information; ///< Information about this room.
};

} // namespace
