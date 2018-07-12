// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class SequenceDialog;
}

class SequenceDialog : public QDialog {
    Q_OBJECT

public:
    explicit SequenceDialog(QWidget* parent = nullptr);
    ~SequenceDialog();

    QKeySequence GetSequence();
    void closeEvent(QCloseEvent*) override;

private:
    std::unique_ptr<Ui::SequenceDialog> ui;
};
