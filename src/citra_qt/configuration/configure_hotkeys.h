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

    void emitHotkeysChanged();

public slots:
    void onInputKeysChanged(QList<QKeySequence> new_key_list);

signals:
    void hotkeysChanged(QList<QKeySequence> new_key_list);

private:
    bool eventFilter(QObject* o, QEvent* e);
    void configure(QModelIndex index);
    bool isUsedKey(QKeySequence key_sequence);
    QList<QKeySequence> getUsedKeyList();

    std::unique_ptr<Ui::ConfigureHotkeys> ui;

    QList<QKeySequence> usedInputKeys;
};
