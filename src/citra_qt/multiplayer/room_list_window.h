// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QLineEdit>
#include <QMainWindow>
#include <QObject>
#include <QPushButton>
#include <QSpinBox>
#include "core/announce_netplay_session.h"
#include "network/network.h"

class RoomListWindow : public QMainWindow {
    Q_OBJECT

public:
    enum Mode {
        Create,
        Join,
    };
    explicit RoomListWindow(Mode mode=Join, QWidget *parent = 0);
    ~RoomListWindow();

private:
    void InitializeWidgets(Mode mode);
    void ConnectWidgetEvents(Mode mode);
    bool CloseOnConfirm();
    bool ConfirmLeaveRoom();
    bool ConfirmCloseRoom();

private slots:
    void OnJoin();
    void OnCreate();

private:
    QSpinBox* port = nullptr;
    QLineEdit* server = nullptr;
    QLineEdit* nickname = nullptr;
    QPushButton* join_button = nullptr;
    std::shared_ptr<Network::RoomMember> room_member;
    std::shared_ptr<Network::Room> room;

    std::unique_ptr<Core::NetplayAnnounceSession> announce_netplay_session;
};
