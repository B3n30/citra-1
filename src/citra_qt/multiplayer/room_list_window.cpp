// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <future>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QtConcurrent/QtConcurrent>
#include "citra_qt/multiplayer/room_list_window.h"
#include "citra_qt/multiplayer/room_view_window.h"
#include "core/announce_netplay_session.h"

#include "common/logging/log.h"

RoomListWindow::RoomListWindow(Mode mode, QWidget* parent) : QMainWindow(parent) {
    room = Network::GetRoom().lock();
    room_member = Network::GetRoomMember().lock();

    announce_netplay_session = std::make_unique<Core::NetplayAnnounceSession>();

    InitializeWidgets(mode);
    ConnectWidgetEvents(mode);

    setWindowTitle(tr("Room List"));
    show();
}

RoomListWindow::~RoomListWindow() {}

void RoomListWindow::InitializeWidgets(Mode mode) {
    QGridLayout* central_widget_layout = new QGridLayout();
    QWidget* central_widget = new QWidget();
    central_widget->setLayout(central_widget_layout);

    QWidget* direct_connection_widget = new QWidget();
    QHBoxLayout* direct_connection_widget_layout = new QHBoxLayout();
    direct_connection_widget->setLayout(direct_connection_widget_layout);
    {
        QLabel* server_label = new QLabel(tr("Room IP"));
        server = new QLineEdit();
        direct_connection_widget_layout->addWidget(server_label);
        direct_connection_widget_layout->addWidget(server);

        QLabel* port_label = new QLabel(tr("Port"));
        port = new QSpinBox();
        port->setRange(1, 65535);
        port->setValue(Network::DefaultRoomPort);
        direct_connection_widget_layout->addWidget(port_label);
        direct_connection_widget_layout->addWidget(port);

        QLabel* nickname_label = new QLabel(tr("Nickname"));
        nickname = new QLineEdit();
        direct_connection_widget_layout->addWidget(nickname_label);
        direct_connection_widget_layout->addWidget(nickname);

        join_button = new QPushButton(tr("Join room"));
        direct_connection_widget_layout->addWidget(join_button);
        // TODO (B3N30): Disable if IP is empty

        if (mode == Create) {
            server_label->setText(tr("Room name"));
            join_button->setText(tr("Create and join"));
        }

        refresh_button = new QPushButton(tr("Refresh"));
        direct_connection_widget_layout->addWidget(refresh_button);
    }
    central_widget_layout->addWidget(direct_connection_widget);
    room_list = new QTreeView();
    item_model = new QStandardItemModel(room_list);
    room_list->setModel(item_model);
    room_list->setAlternatingRowColors(true);
    room_list->setSelectionMode(QHeaderView::SingleSelection);
    room_list->setSelectionBehavior(QHeaderView::SelectRows);
    room_list->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    room_list->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    room_list->setSortingEnabled(true);
    room_list->setEditTriggers(QHeaderView::NoEditTriggers);
    room_list->setUniformRowHeights(true);
    room_list->setContextMenuPolicy(Qt::CustomContextMenu);

    enum {
        COLUMN_NAME,
        COLUMN_IP,
        COLUMN_PORT,
        COLUMN_PLAYERS,
        COLUMN_COUNT, // Number of columns
    };

    item_model->insertColumns(0, COLUMN_COUNT);
    item_model->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
    item_model->setHeaderData(COLUMN_IP, Qt::Horizontal, tr("IP"));
    item_model->setHeaderData(COLUMN_PORT, Qt::Horizontal, tr("Port"));
    item_model->setHeaderData(COLUMN_PLAYERS, Qt::Horizontal, tr("Players"));

    room_list->header()->setStretchLastSection(false);
    room_list->header()->setSectionResizeMode(COLUMN_PLAYERS, QHeaderView::Stretch);
    central_widget_layout->addWidget(room_list);
    setCentralWidget(central_widget);
}

void RoomListWindow::ConnectWidgetEvents(Mode mode) {
    switch (mode) {
    case Join:
        connect(join_button, SIGNAL(clicked()), this, SLOT(OnJoin()));
        break;
    case Create:
        connect(join_button, SIGNAL(clicked()), this, SLOT(OnCreate()));
        break;
    }
    connect(refresh_button, SIGNAL(clicked()), this, SLOT(OnRefresh()));
    connect(this, SIGNAL(RefreshRoomList()), this, SLOT(OnRefreshRoomList()));
}

void RoomListWindow::OnJoin() {
    if (!CloseOnConfirm())
        return;
    const std::string room_ip(server->text().toStdString());
    const std::string str_nickname(nickname->text().toStdString());
    room_member->Join(str_nickname, room_ip.c_str(), port->value());
    RoomViewWindow* room_view_window = new RoomViewWindow();
    room_view_window->show();
    // TODO(B3N30): Close this window
}

void RoomListWindow::OnCreate() {
    if (!CloseOnConfirm())
        return;
    const std::string room_name(server->text().toStdString());
    if (!room->Create(room_name, "", port->value())) {
        QMessageBox::critical(this, tr("Citra"),
                              tr("Failed to create room. Check your network settings"),
                              QMessageBox::Ok, QMessageBox::Ok);
        return;
    }
    const std::string str_nickname(nickname->text().toStdString());
    room_member->Join(str_nickname, "127.0.0.1", port->value());
    announce_netplay_session->Start();
    RoomViewWindow* room_view_window = new RoomViewWindow();
    room_view_window->show();
}

void RoomListWindow::OnRefresh() {
    LOG_DEBUG(Network, "Refresh clicked");
    room_list_future = announce_netplay_session->GetRoomList([&](){emit RefreshRoomList();});
}

void RoomListWindow::OnRefreshRoomList() {
    item_model->removeRows(0, item_model->rowCount());
    NetplayAnnounce::RoomList new_room_list = room_list_future.get();
    for (const auto& room : new_room_list) {
        QList<QStandardItem*> l;
        QString port;
        port.sprintf("%u", room.port);
        QString players;
        players.sprintf("%lu/%u", room.members.size(), room.max_player);
        std::vector<std::string> elements = {room.name, room.ip, port.toStdString(),
                                                players.toStdString()};
        for (auto& item : elements) {
            QStandardItem* child = new QStandardItem(QString::fromStdString(item));
            child->setEditable(false);
            l.append(child);
        }
        item_model->invisibleRootItem()->appendRow(l);
    }

    // TODO(B3N30): Restore row selection
}

bool RoomListWindow::CloseOnConfirm() {
    if (room->GetState() != Network::Room::State::Closed) {
        if (!ConfirmCloseRoom()) {
            return false;
        }
        announce_netplay_session->Stop();
        announce_netplay_session.reset();
        room_member->Leave();
        room->Destroy();
    } else if (room_member->IsConnected()) {
        if (!ConfirmLeaveRoom()) {
            return false;
        }
        room_member->Leave();
    }
    return true;
}

bool RoomListWindow::ConfirmLeaveRoom() {
    auto answer =
        QMessageBox::question(this, tr("Citra"),
                              tr("Are you sure you want to leave this room? Your simulated WiFi "
                                 "connection to all other members will be lost."),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

bool RoomListWindow::ConfirmCloseRoom() {
    auto answer =
        QMessageBox::question(this, tr("Citra"),
                              tr("Are you sure you want to close this room? The simulated WiFi "
                                 "connections of all members will be lost."),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}
