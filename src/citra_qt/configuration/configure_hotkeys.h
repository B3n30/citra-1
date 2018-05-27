// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QStandardItemModel>
#include <QWidget>
#include "common/param_package.h"
#include "core/settings.h"

class QTimer;

namespace Ui {
class ConfigureHotkeys;
}

class ConfigureHotkeys : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureHotkeys(QWidget* parent = nullptr);
    ~ConfigureHotkeys();

    void applyConfiguration();

private:
    bool eventFilter(QObject* o, QEvent* e);
    void configure(QModelIndex);
    bool isUsedKey(QKeySequence key_sequence);

    std::unique_ptr<Ui::ConfigureHotkeys> ui;

    std::array<Common::ParamPackage, Settings::NativeButton::NumButtons> buttons_param;
    std::array<Common::ParamPackage, Settings::NativeAnalog::NumAnalogs> analogs_param;

    static const int ANALOG_SUB_BUTTONS_NUM = 5;
    static const std::array<std::string, ANALOG_SUB_BUTTONS_NUM> analog_sub_buttons;
};
