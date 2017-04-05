// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QPushButton>

#include "net_core/net_core.h"
#include "citra_qt/multiplayer/start_server_dialog.h"
#include "ui_start_server.h"

StartServerDialog::StartServerDialog(QWidget* parent) : QDialog(parent), ui(new Ui::StartServerDialog) {
    ui->setupUi(this);
    Room* room = NetCore::getRoom();
    if (room->GetState() == Room::State::Open) {
        ui->edit_room_name->setEnabled(false);
        ui->spin_port->setEnabled(false);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Destroy");
    } else {
        ui->edit_room_name->setEnabled(true);
        ui->spin_port->setEnabled(true);
        ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Create");
    }
}

StartServerDialog::~StartServerDialog() {}

void StartServerDialog::onAccept() {
    Room* room = NetCore::getRoom();
    if (room->GetState() == Room::State::Open) {
        room->Destroy();
    } else {
        const std::string room_name(ui->edit_room_name->text().toStdString());
        room->Create(room_name,"",ui->spin_port->value());
    }
}
