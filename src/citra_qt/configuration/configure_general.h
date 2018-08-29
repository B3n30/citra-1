// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <memory>
#include <QWidget>

class HotkeyRegistry;

namespace Ui {
class ConfigureGeneral;
}

class ConfigureGeneral : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureGeneral(QWidget* parent = nullptr);
    ~ConfigureGeneral();

    void PopulateHotkeyList(const HotkeyRegistry& registry);
    void applyConfiguration(HotkeyRegistry& registry);
    void retranslateUi();
    void EmitHotkeysChanged();

public slots:
    void OnInputKeysChanged(QList<QKeySequence> new_key_list);

signals:
    void languageChanged(const QString& locale);
    void HotkeysChanged(QList<QKeySequence> new_key_list);
    void InputKeysChanged(QList<QKeySequence> new_key_list);

private slots:
    void OnLanguageChanged(int index);

private:
    void setConfiguration();

    std::unique_ptr<Ui::ConfigureGeneral> ui;
};
