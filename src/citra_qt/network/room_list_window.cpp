// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/network/room_list_window.h"
#include "citra_qt/network/room_view_window.h"

#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

RoomListWindow::RoomListWindow(Mode mode, QWidget* parent) : QMainWindow(parent) {

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
        port->setValue(DefaultRoomPort);
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
    }
    central_widget_layout->addWidget(direct_connection_widget);
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
}

void RoomListWindow::OnJoin() {
    if (!CloseOnConfirm())
        return;
    const std::string room_ip(server->text().toStdString());
    const std::string str_nickname(nickname->text().toStdString());
    // TODO(B3N30): Connect to the room
    RoomViewWindow* room_view_window = new RoomViewWindow();
    room_view_window->show();
}

void RoomListWindow::OnCreate() {
    if (!CloseOnConfirm())
        return;
    const std::string room_name(server->text().toStdString());
    const std::string str_nickname(nickname->text().toStdString());
    // TODO(B3N30): create and join the room
    RoomViewWindow* room_view_window = new RoomViewWindow();
    room_view_window->show();
}

bool RoomListWindow::CloseOnConfirm() {
    // TODO(B3N30): Disconnect and close the room if necessary
    return true;
}

bool RoomListWindow::ConfirmLeaveRoom() {
    auto answer = QMessageBox::question(
        this, tr("Citra"), tr("Are you sure you want to leave this room? Your simulated WiFi "
                              "connection to all other members will be lost."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

bool RoomListWindow::ConfirmCloseRoom() {
    auto answer = QMessageBox::question(
        this, tr("Citra"), tr("Are you sure you want to close this room? The simulated WiFi "
                              "connections of all members will be lost."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}
