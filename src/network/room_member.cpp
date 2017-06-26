// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
#include "network/packet.h"
#include "network/room_member.h"

namespace Network {

const uint32_t ConnectionTimeout = 5000; // ms

RoomMember::RoomMember() {
    client = enet_host_create(nullptr, 1, NumChannels, 0, 0);
    ASSERT_MSG(client != nullptr, "Could not create client");
}

RoomMember::~RoomMember() {
    ASSERT_MSG(!IsConnected(), "RoomMember is being destroyed while connected");
    enet_host_destroy(client);
}

template<typename T>
RoomMember::Connection<T> RoomMember::Connect(std::function<void(const T&)> callback) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    Connection<T> connection;
    connection =  std::make_shared<std::function<void(const T&)> >(callback);
    callbacks.Get<T>().insert(connection);
    return connection;
}

template<typename T>
void RoomMember::Disconnect(Connection<T> connection) {
    std::lock_guard<std::mutex> lock(callback_mutex);
    callbacks.Get<T>().erase(connection.callback);
}

template<typename T>
void RoomMember::Invoke(const T& data)
{
    CallbackSet<T> callback_set;
    {
        std::lock_guard<std::mutex> lock(callback_mutex);
        callback_set = callbacks.Get<T>();
    }
    for(auto const& callback: callback_set)
        (*callback)(data);
}

void RoomMember::Send(Packet& packet) {
    ENetPacket* enetPacket = enet_packet_create(packet.GetData(), packet.GetDataSize(),  ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(server, 0, enetPacket);
    enet_host_flush(client);
}

void RoomMember::SendJoinRequest(const std::string& nickname, const MacAddress& preferred_mac) {
    Packet packet;
    packet << static_cast<MessageID>(IdJoinRequest);
    packet << nickname;
    packet << preferred_mac;
    Send(packet);
}

void RoomMember::HandleWifiPackets(const ENetEvent* event) {
    WifiPacket wifi_packet{};
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);

    // Ignore the first byte, which is the message id.
    packet.IgnoreBytes(sizeof(MessageID));

    // Parse the WifiPacket from the BitStream
    uint8_t frame_type;
    packet >> frame_type;
    WifiPacket::PacketType type = static_cast<WifiPacket::PacketType>(frame_type);

    wifi_packet.type = type;
    packet >> wifi_packet.channel;
    packet >> wifi_packet.transmitter_address;
    packet >> wifi_packet.destination_address;

    uint32_t data_length;
    packet >> data_length;

    std::vector<uint8_t> data(data_length);
    packet >> data;

    wifi_packet.data = std::move(data);
    Invoke(wifi_packet);
}

void RoomMember::HandleRoomInformationPacket(const ENetEvent* event) {
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);

    // Ignore the first byte, which is the message id.
    packet.IgnoreBytes(sizeof(MessageID));

    RoomInformation info{};
    packet >> info.name;
    packet >> info.member_slots;
    room_information.name = info.name;
    room_information.member_slots = info.member_slots;

    uint32_t num_members;
    packet >> num_members;
    member_information.resize(num_members);

    for (auto& member : member_information) {
        packet >> member.nickname;
        packet >> member.mac_address;
        packet >> member.game_name;
    }
    Invoke(info);
}

void RoomMember::HandleJoinPacket(const ENetEvent* event) {
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);

    // Ignore the first byte, which is the message id.
    packet.IgnoreBytes(sizeof(MessageID));

    //Parse the MAC Address from the BitStream
    packet >> mac_address;
    Invoke(State::Joined);
}

void RoomMember::HandleChatPacket(const ENetEvent* event) {
    Packet packet;
    packet.Append(event->packet->data, event->packet->dataLength);

    // Ignore the first byte, which is the message id.
    packet.IgnoreBytes(sizeof(MessageID));

    ChatEntry chat_entry{};
    packet >> chat_entry.nickname;
    packet >> chat_entry.message;
    Invoke(chat_entry);
}

void RoomMember::ReceiveLoop() {
    // Receive packets while the connection is open
    while (IsConnected()) {
        std::lock_guard<std::mutex> lock(network_mutex);

        ENetEvent event;
        while (enet_host_service(client, &event, 1000) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                switch (event.packet->data[0]) {
                case IdChatMessage:
                    HandleChatPacket(&event);
                    break;
                case IdWifiPacket:
                    HandleWifiPackets(&event);
                    break;
                case IdRoomInformation:
                    HandleRoomInformationPacket(&event);
                    break;
                case IdNameCollision:
                    state = State::NameCollision;
                    Invoke(State::NameCollision);
                    break;
                case IdMacCollision:
                    state = State::MacCollision;
                    Invoke(State::MacCollision);
                    break;
                case IdJoinSuccess:
                    // The join request was successful, we are now in the room.
                    // If we joined successfully, there must be at least one client in the room: us.
                    ASSERT_MSG(GetMemberInformation().size() > 0, "We have not yet received member information.");
                    HandleJoinPacket(&event);    // Get the MAC Address for the client
                    state = State::Joined;
                    Invoke(State::Joined);
                    break;
                default:
                    break;
                }
                enet_packet_destroy(event.packet);
            }
        }
    }
};

void RoomMember::Join(const std::string& nickname, const std::string& server, uint16_t server_port,
                      uint16_t client_port) {
    ENetAddress address;
    enet_address_set_host(&address, server.c_str());
    address.port = server_port;

    this->server = enet_host_connect(client, &address, NumChannels, 0);

    if (this->server == nullptr) {
        state = State::Error;
        Invoke(State::Error);
        return;
    }

    ENetEvent event;
    int net = enet_host_service(client, &event, ConnectionTimeout);
    if (net > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        this->nickname = nickname;
        state = State::Joining;
        Invoke(State::Joining);
        receive_thread = std::make_unique<std::thread>(&RoomMember::ReceiveLoop, this);
        SendJoinRequest(nickname);
    } else {
        state = State::CouldNotConnect;
        Invoke(State::CouldNotConnect);
    }
}

bool RoomMember::IsConnected() const {
    return state == State::Joining || state == State::Joined;
}

void RoomMember::Leave() {
    ASSERT_MSG(receive_thread != nullptr, "Must be in a room to leave it.");
    {
        std::lock_guard<std::mutex> lock(network_mutex);
        enet_peer_disconnect(server, 0);
        state = State::Idle;
        Invoke(State::Idle);
    }

    receive_thread->join();
    receive_thread.reset();
    enet_peer_reset(server);
}


template<>
RoomMember::CallbackSet<WifiPacket>& RoomMember::Callbacks::Get() {
    return callback_set_wifi_packet;
}

template<>
RoomMember::CallbackSet<RoomInformation>& RoomMember::Callbacks::Get() {
    return callback_set_room_information;
}

template<>
RoomMember::CallbackSet<ChatEntry>& RoomMember::Callbacks::Get() {
    return callback_set_chat_messages;
}

template<>
RoomMember::CallbackSet<RoomMember::State>& RoomMember::Callbacks::Get() {
    return callback_set_state;
}

} // namespace
