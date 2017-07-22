// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "common/common_types.h"
#include "network/room.h"

namespace Network {

/// Information about the received WiFi packets.
/// Acts as our own 802.11 header.
struct WifiPacket {
    enum class PacketType : u8 { Beacon, Data, Authentication, AssociationResponse };
    PacketType type;      ///< The type of 802.11 frame.
    std::vector<u8> data; ///< Raw 802.11 frame data, starting at the management frame header
                          /// for management frames.
    MacAddress transmitter_address; ///< Mac address of the transmitter.
    MacAddress destination_address; ///< Mac address of the receiver.
    u8 channel;                     ///< WiFi channel where this frame was transmitted.
};

/// Represents a chat message.
struct ChatEntry {
    std::string nickname; ///< Nickname of the client who sent this message.
    std::string message;  ///< Body of the message.
};

/**
 * This is what a client [person joining a server] would use.
 * It also has to be used if you host a game yourself (You'd create both, a Room and a
 * RoomMembership for yourself)
 */
class RoomMember final {
public:
    enum class State : u8 {
        Idle,    ///< Default state
        Error,   ///< Some error [permissions to network device missing or something]
        Joining, ///< The client is attempting to join a room.
        Joined,  ///< The client is connected to the room and is ready to send/receive packets.
        LostConnection, ///< Connection closed

        // Reasons why connection was rejected
        NameCollision,  ///< Somebody is already using this name
        MacCollision,   ///< Somebody is already using that mac-address
        WrongVersion,   ///< The room version is not the same as for this RoomMember
        CouldNotConnect ///< The room is not responding to a connection attempt
    };

    struct MemberInformation {
        std::string nickname;   ///< Nickname of the member.
        GameInfo game_info;     ///< Name of the game they're currently playing, or empty if they're
                                /// not playing anything.
        MacAddress mac_address; ///< MAC address associated with this member.
    };
    using MemberList = std::vector<MemberInformation>;

    // The handle for the callback functions
    template <typename T>
    using Connection = std::shared_ptr<std::function<void(const T&)>>;

    RoomMember();
    ~RoomMember();

    /**
     * Returns the status of our connection to the room.
     */
    State GetState() const;

    /**
     * Returns information about the members in the room we're currently connected to.
     */
    const MemberList& GetMemberInformation() const;

    /**
     * Returns the nickname of the RoomMember.
     */
    const std::string& GetNickname() const;

    /**
     * Returns the MAC address of the RoomMember.
     */
    const MacAddress& GetMacAddress() const;

    /**
     * Returns information about the room we're currently connected to.
     */
    RoomInformation GetRoomInformation() const;

    /**
     * Returns whether we're connected to a server or not.
     */
    bool IsConnected() const;

    /**
     * Attempts to join a room at the specified address and port, using the specified nickname.
     * This may fail if the username is already taken.
     */
    void Join(const std::string& nickname, const char* server_addr = "127.0.0.1",
              const u16 serverPort = DefaultRoomPort, const u16 clientPort = 0,
              const MacAddress& preferred_mac = NoPreferredMac);

    /**
     * Sends a WiFi packet to the room.
     * @param packet The WiFi packet to send.
     */
    void SendWifiPacket(const WifiPacket& packet);

    /**
     * Sends a chat message to the room.
     * @param message The contents of the message.
     */
    void SendChatMessage(const std::string& message);

    /**
     * Sends the current game info to the room.
     * @param game_info The game information.
     */
    void SendGameInfo(const GameInfo& game_info);

    // The handle for the callback functions
    template <typename T>
    using Connection = std::shared_ptr<std::function<void(const T&)>>;
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

    Connection<State> ConnectOnStateChanged(std::function<void(const State&)> callback);
    Connection<WifiPacket> ConnectOnWifiPacketReceived(
        std::function<void(const WifiPacket&)> callback);
    Connection<RoomInformation> ConnectOnRoomInformationChanged(
        std::function<void(const RoomInformation&)> callback);
    Connection<ChatEntry> ConnectOnChatMessageRecieved(
        std::function<void(const ChatEntry&)> callback);

    /**
     * Disconnect a function from the events.
     */
    template <typename T>
    void Disconnect(Connection<T> connection);

    /**
     * Leaves the current room.
     */
    void Leave();

private:
    class RoomMemberImpl;
    std::unique_ptr<RoomMemberImpl> room_member_impl;
};

} // namespace Network
