// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <string>
#include "network/room.h"

namespace Network {

// This is what a client [person joining a server] would use.
// It also has to be used if you host a game yourself (You'd create both, a Room and a
// RoomMembership for yourself)
class RoomMember final {
public:
    enum class State {
        Idle,           ///< Default state
        Error,          ///< Some error [permissions to network device missing or something]
        Joining,        ///< The client is attempting to join a room.
        Joined,         ///< The client is connected to the room and is ready to send/receive packets.
        LostConnection, ///< Connection closed

        // Reasons why connection was rejected
        NameCollision,  ///< Somebody is already using this name
        MacCollision,   ///< Somebody is already using that mac-address
        CouldNotConnect ///< The room is not responding to a connection attempt
    };

    RoomMember();
    ~RoomMember();

    /**
     * Returns the status of our connection to the room.
     */
    State GetState() const {
        return state;
    };

    /**
     * Returns whether we're connected to a server or not.
     */
    bool IsConnected() const;

    /**
     * Attempts to join a room at the specified address and port, using the specified nickname.
     * This may fail if the username is already taken.
     */
    void Join(const std::string& nickname, const std::string& server = "127.0.0.1",
              const uint16_t serverPort = DefaultRoomPort, const uint16_t clientPort = 0);

    /**
     * Leaves the current room.
     */
    void Leave();

private:
    ENetHost* client;                      ///< ENet network interface.
    ENetPeer* server;                      ///< The server peer the client is connected to
    std::atomic<State> state{State::Idle}; ///< Current state of the RoomMember.

    std::string nickname; ///< The nickname of this member.
};

} // namespace
