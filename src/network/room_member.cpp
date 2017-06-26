// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/assert.h"
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

void RoomMember::Join(const std::string& nickname, const std::string& server, uint16_t server_port,
                      uint16_t client_port) {
    ENetAddress address;
    enet_address_set_host(&address, server.c_str());
    address.port = server_port;

    this->server = enet_host_connect(client, &address, NumChannels, 0);

    if (this->server == nullptr) {
        state = State::Error;
        return;
    }

    ENetEvent event;
    int net = enet_host_service(client, &event, ConnectionTimeout);
    if (net > 0 && event.type == ENET_EVENT_TYPE_CONNECT) {
        this->nickname = nickname;
        state = State::Joining;
        // TODO(B3N30): Send a join request with the nickname to the server
        // TODO(B3N30): Start the receive thread
    } else {
        state = State::CouldNotConnect;
    }
}

bool RoomMember::IsConnected() const {
    return state == State::Joining || state == State::Joined;
}

void RoomMember::Leave() {
    enet_peer_disconnect(server, 0);
    state = State::Idle;
    // TODO(B3N30): Close the receive thread
    enet_peer_reset(server);
}

} // namespace
