// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <future>
#include <memory>
#include <QWidget>

namespace Ui {
class ConfigureWeb;
}

class ConfigureWeb : public QWidget {
    Q_OBJECT

public:
    explicit ConfigureWeb(QWidget* parent = nullptr);
    ~ConfigureWeb();

    void applyConfiguration();

public slots:
    void refreshTelemetryID();
    void loginChanged();
    void verifyLogin();
    void onLoginVerified();

signals:
    void loginVerified();

private:
    void setConfiguration();

    bool user_verified = true;
    std::future<bool> verified;

    std::unique_ptr<Ui::ConfigureWeb> ui;
};
