// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QDialog>
#include "core/cheats/cheats.h"

class QComboBox;
class QLineEdit;
class QWidget;
namespace Ui {
class CheatDialog;
class NewCheatDialog;
} // namespace Ui

class CheatDialog : public QDialog {
    Q_OBJECT

public:
    explicit CheatDialog(QWidget* parent = nullptr);
    ~CheatDialog();

private:
    std::unique_ptr<Ui::CheatDialog> ui;
    int current_row = -1;
    bool selection_changing = false;
    std::vector<std::shared_ptr<Cheats::CheatBase>> cheats;

    void LoadCheats();

private slots:
    void OnCancel();
    void OnRowSelected(int row, int column);
    void OnCheckChanged(int state);
};
