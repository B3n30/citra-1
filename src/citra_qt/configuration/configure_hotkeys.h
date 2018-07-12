// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QStandardItemModel>
#include <QWidget>
#include "common/param_package.h"
#include "core/settings.h"

namespace Ui {
class ConfigureHotkeys;
}

class ConfigureHotkeys : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureHotkeys(QWidget* parent = nullptr);
    ~ConfigureHotkeys();

    void applyConfiguration();
    void retranslateUi();

    void EmitHotkeysChanged();

public slots:
    void OnInputKeysChanged(QList<QKeySequence> new_key_list);

signals:
    void HotkeysChanged(QList<QKeySequence> new_key_list);

private:
    void Configure(QModelIndex index);
    bool IsUsedKey(QKeySequence key_sequence);
    QList<QKeySequence> GetUsedKeyList();

    std::unique_ptr<Ui::ConfigureHotkeys> ui;

    QList<QKeySequence> input_keys_list;
};
