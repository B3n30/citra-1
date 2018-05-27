// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/util/sequence_dialog/sequence_dialog.h"
#include "ui_sequence_dialog.h"

SequenceDialog::SequenceDialog(QWidget* parent)
    : QDialog(parent), ui(new Ui::SequenceDialog ) {
    ui->setupUi(this);

    connect(ui->ok_button, &QPushButton::clicked, this, &QDialog::close);
}

QKeySequence SequenceDialog::getSequence() {
    return QKeySequence(ui->key_sequence->keySequence()[0]);
}

SequenceDialog::~SequenceDialog() {
    delete ui;
}
