// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QPushButton>
#include "citra_qt/util/sequence_dialog/sequence_dialog.h"
#include "ui_sequence_dialog.h"

SequenceDialog::SequenceDialog(QWidget* parent)
    : QDialog(parent), ui(std::make_unique<Ui::SequenceDialog>()) {
    ui->setupUi(this);

    connect(ui->ok_button, &QPushButton::clicked, this, &QDialog::accept);
    connect(ui->cancel_button, &QPushButton::clicked, this, &QDialog::reject);
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
}

SequenceDialog::~SequenceDialog() = default;

QKeySequence SequenceDialog::GetSequence() {
    return QKeySequence(ui->key_sequence->keySequence()[0]);
}

void SequenceDialog::closeEvent(QCloseEvent*) {
    reject();
}
