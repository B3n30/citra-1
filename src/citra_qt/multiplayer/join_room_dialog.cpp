// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QPushButton>
#include <iostream>

#include "common/logging/log.h"
#include "net_core/net_core.h"
#include "citra_qt/multiplayer/join_room_dialog.h"
#include "ui_join_room.h"

JoinRoomDialog::JoinRoomDialog(QWidget* parent) : QDialog(parent), ui(new Ui::JoinRoomDialog) {
    ui->setupUi(this);
    RoomMember* room_member = NetCore::getRoomMember();
    if (room_member->IsConnected()) {
        ui->edit_nickname->setEnabled(false);
        ui->edit_room_ip->setEnabled(false);
        ui->spin_port->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Leave");
    } else {
        ui->edit_nickname->setEnabled(true);
        ui->edit_room_ip->setEnabled(true);
        ui->spin_port->setEnabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Join");
    }
}

JoinRoomDialog::~JoinRoomDialog() {}

void JoinRoomDialog::onAccept() {
    RoomMember* room_member = NetCore::getRoomMember();
    if (room_member->IsConnected()) {
        room_member->Leave();
    } else {
        const std::string nickname(ui->edit_nickname->text().toStdString());
        const std::string room_ip(ui->edit_room_ip->text().toStdString());
        room_member->Join(nickname, room_ip, ui->spin_port->value());
    }
}
