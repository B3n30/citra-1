// Copyright 2014 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <iostream>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

#ifdef _WIN32
// windows.h needs to be included before shellapi.h
#include <windows.h>

#include <shellapi.h>
#endif

//#include "core/announce_netplay_session.h"
#include "network/network.h"


static void PrintHelp(const char* argv0) {
    std::cout << "Usage: " << argv0
              << " <room_name> <port>\n";
}


/// Application entry point
int main(int argc, char** argv) {
    if (argc != 3) {
        PrintHelp(argv[0]);
        return 0;
    }

    std::string room_name = argv[1];
    u16 port = atoi(argv[2]);
    Network::Init();
    if (std::shared_ptr<Network::Room> room = Network::GetRoom().lock()) {
        if(!room->Create(room_name, "", port)) {
            std::cout << "Failed to create room: \n";
            return -1;
        }
//        auto announce_netplay_session = std::make_unique<Core::NetplayAnnounceSession>();
//        announce_netplay_session->Start();
        std::cout << "press any key to quit\n";
        while(room->GetState() == Network::Room::State::Open) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            std::string in;
            std::cin >> in;
            if (in.size() > 0) {
//                announce_netplay_session->Stop();
//                announce_netplay_session.reset();
                room->Destroy();
                Network::Shutdown();
                return 0;
            }
        }
//        announce_netplay_session->Stop();
//        announce_netplay_session.reset();
        room->Destroy();
    }
    Network::Shutdown();
    return 0;

}
