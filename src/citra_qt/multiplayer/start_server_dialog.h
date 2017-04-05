// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>

namespace Ui {
class StartServerDialog;
}

class StartServerDialog : public QDialog {
    Q_OBJECT

public:
    explicit StartServerDialog(QWidget* parent);
    ~StartServerDialog();

    void onAccept();

private:
    std::unique_ptr<Ui::StartServerDialog> ui;
};
