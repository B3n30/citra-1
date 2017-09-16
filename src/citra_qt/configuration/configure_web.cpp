// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QMessageBox>
#include "citra_qt/configuration/configure_web.h"
#include "core/settings.h"
#include "core/telemetry_session.h"
#include "ui_configure_web.h"

ConfigureWeb::ConfigureWeb(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureWeb>()) {
    ui->setupUi(this);
    connect(ui->button_regenerate_telemetry_id, &QPushButton::clicked, this,
            &ConfigureWeb::refreshTelemetryID);
    connect(ui->edit_token, &QLineEdit::textChanged, this, &ConfigureWeb::loginChanged);
    connect(ui->edit_username, &QLineEdit::textChanged, this, &ConfigureWeb::loginChanged);
    connect(ui->button_verfiy_login, &QPushButton::clicked, this, &ConfigureWeb::verifyLogin);
    connect(this, SIGNAL(loginVerified()), this, SLOT(onLoginVerified()));

    this->setConfiguration();
}

ConfigureWeb::~ConfigureWeb() {}

void ConfigureWeb::setConfiguration() {
    ui->web_credentials_disclaimer->setWordWrap(true);
    ui->telemetry_learn_more->setOpenExternalLinks(true);
    ui->telemetry_learn_more->setText("<a "
                                      "href='https://citra-emu.org/entry/"
                                      "telemetry-and-why-thats-a-good-thing/'>Learn more</a>");

    ui->web_signup_link->setOpenExternalLinks(true);
    ui->web_signup_link->setText("<a href='https://services.citra-emu.org/'>Sign up</a>");
    ui->web_token_info_link->setOpenExternalLinks(true);
    ui->web_token_info_link->setText(
        "<a href='https://citra-emu.org/wiki/citra-web-service/'>What is my token?</a>");

    ui->toggle_telemetry->setChecked(Settings::values.enable_telemetry);
    ui->edit_username->setText(QString::fromStdString(Settings::values.citra_username));
    ui->edit_token->setText(QString::fromStdString(Settings::values.citra_token));
    ui->label_telemetry_id->setText("Telemetry ID: 0x" +
                                    QString::number(Core::GetTelemetryId(), 16).toUpper());
    ui->button_verfiy_login->setDisabled(true);
    user_verified = true;
}

void ConfigureWeb::applyConfiguration() {
    Settings::values.enable_telemetry = ui->toggle_telemetry->isChecked();
    if (user_verified) {
        Settings::values.citra_username = ui->edit_username->text().toStdString();
        Settings::values.citra_token = ui->edit_token->text().toStdString();
    } else {
        QMessageBox::warning(this, tr("Username and token not verfied"),
                             tr("Username and token were not verified. The changes to your "
                                "username and/or token have not been saved."));
    }
    Settings::Apply();
}

void ConfigureWeb::refreshTelemetryID() {
    const u64 new_telemetry_id{Core::RegenerateTelemetryId()};
    ui->label_telemetry_id->setText("Telemetry ID: 0x" +
                                    QString::number(new_telemetry_id, 16).toUpper());
}

void ConfigureWeb::loginChanged() {
    if (ui->edit_username->text().isEmpty() && ui->edit_token->text().isEmpty()) {
        ui->button_verfiy_login->setDisabled(true);
        user_verified = true;
    } else {
        ui->button_verfiy_login->setEnabled(true);
        user_verified = false;
    }
}

void ConfigureWeb::verifyLogin() {
    verified =
        Core::VerifyLogin(ui->edit_username->text().toStdString(),
                          ui->edit_token->text().toStdString(), [&]() { emit loginVerified(); });
    ui->button_verfiy_login->setDisabled(true);
}

void ConfigureWeb::onLoginVerified() {
    if (verified.get()) {
        user_verified = true;
        QMessageBox::information(this, tr("Verification succeeded."),
                                 tr("Verification succeeded."));
    } else {
        QMessageBox::critical(
            this, tr("Verification failed."),
            tr("Verification failed. Check that you have entered your username and token "
               "correctly, and that your internet connection is working."));
    }
}
