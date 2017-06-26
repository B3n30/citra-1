// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "network/room.h"

namespace Network {

/// Information about the received WiFi packets.
/// Acts as our own 802.11 header.
struct WifiPacket {
    enum class PacketType { Beacon, Data, Management };
    PacketType type;           ///< The type of 802.11 frame, Beacon / Data.
    std::vector<uint8_t> data; ///< Raw 802.11 frame data, starting at the management frame header
                               /// for management frames.
    MacAddress transmitter_address; ///< Mac address of the transmitter.
    MacAddress destination_address; ///< Mac address of the receiver.
    uint8_t channel;                ///< WiFi channel where this frame was transmitted.
};

/// Represents a chat message.
struct ChatEntry {
    std::string nickname; ///< Nickname of the client who sent this message.
    std::string message;  ///< Body of the message.
};

// This is what a client [person joining a server] would use.
// It also has to be used if you host a game yourself (You'd create both, a Room and a
// RoomMembership for yourself)
class RoomMember final {
public:
    enum class State {
        Idle,    ///< Default state
        Error,   ///< Some error [permissions to network device missing or something]
        Joining, ///< The client is attempting to join a room.
        Joined,  ///< The client is connected to the room and is ready to send/receive packets.
        LostConnection, ///< Connection closed

        // Reasons why connection was rejected
        NameCollision,  ///< Somebody is already using this name
        MacCollision,   ///< Somebody is already using that mac-address
        CouldNotConnect ///< The room is not responding to a connection attempt
    };

    struct MemberInformation {
        std::string nickname;   ///< Nickname of the member.
        std::string game_name;  ///< Name of the game they're currently playing, or empty if they're
                                /// not playing anything.
        MacAddress mac_address; ///< MAC address associated with this member.
    };
    using MemberList = std::vector<MemberInformation>;

    // The handle for the callback functions
    template <typename T>
    using Connection = std::shared_ptr<std::function<void(const T&)>>;

    template <typename T>
    using CallbackSet = std::set<Connection<T>>;

    class Callbacks {
    public:
        template <typename T>
        CallbackSet<T>& Get();

    private:
        CallbackSet<WifiPacket> callback_set_wifi_packet;
        CallbackSet<ChatEntry> callback_set_chat_messages;
        CallbackSet<RoomInformation> callback_set_room_information;
        CallbackSet<RoomMember::State> callback_set_state;
    };

    RoomMember();
    ~RoomMember();

    /**
     * Returns the status of our connection to the room.
     */
    State GetState() const {
        return state;
    }

    /**
     * Returns information about the members in the room we're currently connected to.
     */
    const MemberList& GetMemberInformation() const {
        return member_information;
    }

    /**
     * Returns information about the room we're currently connected to.
     */
    RoomInformation GetRoomInformation() const {
        return room_information;
    }

    /**
     * Returns whether we're connected to a server or not.
     */
    bool IsConnected() const;

    /**
     * Register a function to an event. The function wil be called everytime the event occurs.
     * Depending on the type of the parameter the function is only called for the coresponding
     * event.
     * Allowed typenames are allow: WifiPacket, ChatEntry, RoomInformation, RoomMember::State
     * @param callback The function to call
     * @return A Connection used for removing the function from the registered list
     */
    template <typename T>
    Connection<T> Connect(std::function<void(const T&)> callback);

    /**
     * Disconnect a function from the events.
     */
    template <typename T>
    void Disconnect(Connection<T> connection);

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

    std::string nickname;   ///< The nickname of this member.
    MacAddress mac_address; ///< The mac_address of this member.

    /// Information about the clients connected to the same room as us.
    MemberList member_information;
    /// Information about the room we're connected to.
    RoomInformation room_information;

    std::mutex callback_mutex; ///< The mutex used for handling callbacks
    Callbacks callbacks;       ///< All CallbackSets to all events

    std::mutex network_mutex; ///< Mutex that controls access to the `client` variable.

    /// Thread that receives and dispatches network packets
    std::unique_ptr<std::thread> receive_thread;

    /**
     * Sends data to the room. It will be send on channel 0 with flag RELIABLE
     * @param packet The data to send
     */
    void Send(Packet& packet);

    /**
     * Sends a request to the server, asking for permission to join a room with the specified
     * nickname and preferred mac.
     * @params nickname The desired nickname.
     * @params preferred_mac The preferred MAC address to use in the room, the NoPreferredMac tells
     * the server to assign one for us.
     */
    void SendJoinRequest(const std::string& nickname,
                         const MacAddress& preferred_mac = NoPreferredMac);

    /**
     * Extracts a chat entry from a received ENet packet and adds it to the chat queue.
     * @param packet The RakNet packet that was received.
     */
    void HandleChatPacket(const ENetEvent* event);

    /**
     * Extracts a WifiPacket from a received ENet packet and adds it to the proper queue.
     * @param packet The RakNet packet that was received.
     */
    void HandleWifiPackets(const ENetEvent* event);

    /**
     * Extracts RoomInformation and MemberInformation from a received RakNet packet.
     * @param packet The RakNet packet that was received.
     */
    void HandleRoomInformationPacket(const ENetEvent* event);

    /**
     * Extracts a MAC Address from a received ENet packet.
     * @param packet The RakNet packet that was received.
     */
    void HandleJoinPacket(const ENetEvent* event);

    /**
     *  Invokes an envent. Calls all callbacks associated with the event of that data type
     * @param data The data to send to the callback functions
     */
    template <typename T>
    void Invoke(const T& data);

    /// Thread function that will receive and dispatch messages until connection is lost.
    void ReceiveLoop();
};

} // namespace
