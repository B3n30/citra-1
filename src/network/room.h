// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include "enet/enet.h"
#include "network/packet.h"

namespace Network {

const uint16_t DefaultRoomPort = 1234;
const size_t NumChannels = 1; // Number of channels used for the connection

using MacAddress = std::array<uint8_t, 6>;

/// A special MAC address that tells the room we're joining to assign us a MAC address automatically.
const MacAddress NoPreferredMac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

typedef char MessageID;
enum RoomMessageTypes
{
    IdJoinRequest,
    IdJoinSuccess,
    IdRoomInformation,
    IdSetGameName,
    IdWifiPacket,
    IdChatMessage,
    IdNameCollision,
    IdMacCollision
};

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

    struct Member {
        std::string nickname; ///< The nickname of the member.
        std::string game_name; //< The current game of the member
        MacAddress mac_address; ///< The assigned mac address of the member.
        ENetAddress network_address; ///< The network address of the remote peer.
    };

    using MemberList = std::vector<Member>;

    Room(): random_gen(std::random_device()()) { }
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
    std::mt19937 random_gen; ///< Random number generator. Used for GenerateMacAddress

    RoomInformation room_information; ///< Information about this room.
    MemberList members; ///< Information about the members of this room.

    std::unique_ptr<std::thread> room_thread; ///< Thread that receives and dispatches network packets

    /// Thread function that will receive and dispatch messages until the room is destroyed.
    void ServerLoop();

    /**
     * Parses and answers a room join request from a client.
     * Validates the uniqueness of the username and assigns the MAC address
     * that the client will use for the remainder of the connection.
     */
    void HandleJoinRequest(const ENetEvent* event);

    /**
     * Adds the sender information to a chat message and broadcasts this packet
     * to all members including the sender.
     * @param packet The packet containing the message
     */
    void HandleChatPacket(const ENetEvent* event);

    /**
     * Sends the information about the room, along with the list of members
     * to every connected client in the room.
     * The packet has the structure:
     * <MessageID>ID_ROOM_INFORMATION
     * <String> room_name
     * <uint32_t> member_slots: The max number of clients allowed in this room
     * <uint32_t> num_members: the number of currently joined clients
     * This is followed by the following three values for each member:
     * <String> nickname of that member
     * <MacAddress> mac_address of that member
     * <String> game_name of that member
     */
    void BroadcastRoomInformation();

    /**
     * Returns whether the nickname is valid, ie. isn't already taken by someone else in the room.
     */
    bool IsValidNickname(const std::string& nickname) const;

    /**
     * Returns whether the MAC address is valid, ie. isn't already taken by someone else in the room.
     */
    bool IsValidMacAddress(const MacAddress& address) const;

    /**
     * Generates a free MAC address to assign to a new client.
     * The first 3 bytes are the NintendoOUI 0x00, 0x1F, 0x32
     */
    MacAddress GenerateMacAddress();

    /**
     * Sends a ID_ROOM_NAME_COLLISION message telling the client that the name is invalid.
     */
    void SendNameCollision(ENetPeer* client);

    /**
     * Sends a ID_ROOM_MAC_COLLISION message telling the client that the MAC is invalid.
     */
    void SendMacCollision(ENetPeer* client);

    /**
     * Removes the client from the members list if it was in it and announces the change
     * to all other clients.
     */
    void HandleClientDisconnection(ENetPeer* client);

    /**
     * Notifies the member that its connection attempt was successful,
     * and it is now part of the room.
     */
    void SendJoinSuccess(ENetPeer* client);
};

} // namespace
