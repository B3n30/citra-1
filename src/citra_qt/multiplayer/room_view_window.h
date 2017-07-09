// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>
#include "network/network.h"

class RoomViewWindow : public QMainWindow {
    Q_OBJECT

public:
    Network::RoomMember::State state;
    Network::ChatEntry current_message;
    Network::RoomInformation room_information;
    explicit RoomViewWindow(QWidget *parent = 0);
    ~RoomViewWindow();

private:
    void InitializeWidgets();
    bool ConfirmLeaveRoom();
    bool ConfirmCloseRoom();
    void closeEvent(QCloseEvent* event);
    void ConnectWidgetEvents();
    void AddConnectionMessage(QString message);
    void ConnectRoomEvents();
    void SetUiState(bool connected);

private slots:
    void OnSay();
    void OnMessagesReceived();
    void OnStateChange();
    void OnConnected();
    void OnDisconnected();
    void UpdateMemberList();

    void InvokeOnStateChanged(const Network::RoomMember::State& state);
    void InvokeOnRoomChanged(const Network::RoomInformation& room_information);
    void InvokeOnMessagesReceived(const Network::ChatEntry& message);

private:
    QWidget* chat_widget = nullptr;
    QGridLayout* grid_layout = nullptr;
    QTextEdit* chat_log = nullptr;
    QTreeView* member_list = nullptr;
    QLineEdit* say_line_edit = nullptr;
    QPushButton* say_button = nullptr;
    QStandardItemModel* item_model = nullptr;
    QLabel* status_bar_label = nullptr;
    std::shared_ptr<Network::RoomMember> room_member;
    std::shared_ptr<Network::Room> room;
};
