// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <thread>
#include "common/common_types.h"
#include "common/netplay_announce.h"

namespace Core {

/**
 * Instruments NetplayAnnounce::Backend.
 * Creates a thread that regularly updates the room information and submits them
 * An async get of room information is also possible
 */
class NetplayAnnounceSession : NonCopyable {
public:
    NetplayAnnounceSession();
    ~NetplayAnnounceSession();

    void Start();
    void Stop();

    /*
     *  Returns a list of all room information the backend got
     */
    std::future<NetplayAnnounce::RoomList> GetRoomList();

private:
    std::atomic<bool> announce{true};
    std::unique_ptr<std::thread> netplay_announce_thread;

    std::unique_ptr<NetplayAnnounce::Backend> backend; ///< Backend interface that logs fields

    void AnnounceNetplayLoop();
};

} // namespace Core
