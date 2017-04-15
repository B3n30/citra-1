// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <array>
#include <deque>
#include <string>
#include <vector>

namespace NetCore {
namespace Local {

const uint16_t DefaultPort = 1234;

using MacAddress = std::array<uint8_t, 6>;
const MacAddress NoPreferredMac = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const MacAddress BroadcastMac = NoPreferredMac;

struct MemberInformation {
    std::string nickname;      // Nickname of the member.
    std::string game_name;     // Name of the game they're currently playing, or empty if they're not playing anything.
    MacAddress mac_address;    // MAC address associated with this member.
};
using MemberList = std::vector<MemberInformation>;

struct RoomInformation {
    std::string name;          // Name of the room
    uint32_t member_slots;     // Maximum number of members in this room
};

/// Information about the received WiFi packets.
/// Acts as our own 802.11 header.
struct WifiPacket {
    enum class PacketType {
        Beacon,
        ConnectionRequest,
        ConnectionResponse,
        Data
    };
    PacketType type;                    ///< The type of 802.11 frame, Beacon / Data.
    std::vector<uint8_t> data;          ///< Raw 802.11 frame data, starting at the management frame header for management frames.
    MacAddress transmitter_address;     ///< Mac address of the transmitter.
    MacAddress destination_address;     ///< Mac address of the receiver.
    uint8_t data_channel;               ///< The Data Channel used by NWM_UDS::SendTo and Bind. Works like a port.
                                        ///  Only used by Packets of Type Data
};

/// Represents a chat message.
struct ChatEntry {
    std::string nickname;    ///< Nickname of the client who sent this message.
    std::string message;     ///< Body of the message.
};

enum class MemberState {
    Idle,     // Default state
    Error,    // Some error [permissions to network device missing or something]
    Joining,  // The client is attempting to join a room.
    Joined,   // The client is connected to the room and is ready to send/receive packets.
    Leaving,  // The client is about to close the connection

    // Reasons for connection loss
    LostConnection,

    // Reasons why connection was rejected
    RoomFull,        // The room is full and is not accepting any more connections.
    RoomDestroyed,   // Unknown reason, server not reachable or not responding for w/e reason
    NameCollision,   // Somebody is already using this name
    MacCollision,    // Somebody is already using that mac-address
    WrongVersion,    // The version does not match.
};


enum class RoomState {
    Open,   // The room is open and ready to accept connections.
    Closed, // The room is not opened and can not accept connections.
};

/// Initialise NetCore::Local
bool Init();

/// Shutdown NetCore::Local
void Shutdown();

/**
* Returns the status of our connection to the room.
*/
MemberState GetMemberState();

/**
* Gets the current state of the room.
*/
RoomState GetRoomState();

/**
* Returns whether we're connected to a room or not.
*/
bool IsConnected();

/**
* Returns information about the members in the room we're currently connected to.
*/
MemberList GetMemberInformation();

/**
* Returns information about the room we're currently connected to.
*/
RoomInformation GetRoomInformation();

/**
* Returns a list of received chat messages since the last call.
*/
std::deque<ChatEntry> PopChatEntries();

/**
* Returns a list of received 802.11 frames from the specified sender
* matching the type since the last call.
*/
std::deque<WifiPacket> PopWifiPackets(WifiPacket::PacketType type, const MacAddress& sender = NoPreferredMac);

/**
* Sends a chat message to the room.
* @param message The contents of the message.
*/
void SendChatMessage(const std::string& message);

/**
* Sends a WiFi packet to the room.
* @param packet The WiFi packet to send.
*/
void SendWifiPacket(const WifiPacket& packet);

/**
* Returns a string with informations about the connection
*/
std::string GetStatistics();

/**
* Returns the latest ping to the room
*/
int GetPing();

/**
* Attempts to join a room at the specified address and port, using the specified nickname.
* This may fail if the username is already taken.
*/
void Join(const std::string& nickname, const std::string& server = "127.0.0.1",
          const uint16_t serverPort = DefaultPort, const uint16_t clientPort = 0);

/**
* Leaves the current room.
*/
void Leave();

/**
* Creates a room to join to. Returns true on success. Will bind to default address if server is empty string.
*/
void Create(const std::string& name, const std::string& server = "", uint16_t server_port = DefaultPort);

/**
* Destroys the room
*/
void Destroy();

} // namespace
} // namespace
