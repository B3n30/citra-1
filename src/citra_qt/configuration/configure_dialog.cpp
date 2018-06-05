// Copyright 2016 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "citra_qt/configuration/config.h"
#include "citra_qt/configuration/configure_dialog.h"
#include "citra_qt/hotkeys.h"
#include "core/settings.h"
#include "ui_configure.h"

ConfigureDialog::ConfigureDialog(QWidget* parent, const HotkeyRegistry& registry)
    : QDialog(parent), ui(new Ui::ConfigureDialog) {
    ui->setupUi(this);
    ui->generalTab->PopulateHotkeyList(registry);
    this->setConfiguration();
    connect(ui->generalTab, &ConfigureGeneral::languageChanged, this,
            &ConfigureDialog::onLanguageChanged);
    connect(ui->inputTab, &ConfigureInput::inputKeysChanged, ui->hotkeysTab,
            &ConfigureHotkeys::onInputKeysChanged);
    connect(ui->hotkeysTab, &ConfigureHotkeys::hotkeysChanged, ui->inputTab,
            &ConfigureInput::onHotkeysChanged);

    // Synchronise lists upon initialisation
    ui->inputTab->emitInputKeysChanged();
    ui->hotkeysTab->emitHotkeysChanged();
}

ConfigureDialog::~ConfigureDialog() = default;

void ConfigureDialog::setConfiguration() {}

void ConfigureDialog::applyConfiguration() {
    ui->generalTab->applyConfiguration();
    ui->hotkeysTab->applyConfiguration();
    ui->systemTab->applyConfiguration();
    ui->inputTab->applyConfiguration();
    ui->graphicsTab->applyConfiguration();
    ui->audioTab->applyConfiguration();
    ui->cameraTab->applyConfiguration();
    ui->debugTab->applyConfiguration();
    ui->webTab->applyConfiguration();
    Settings::Apply();
    Settings::LogSettings();
}

void ConfigureDialog::onLanguageChanged(const QString& locale) {
    emit languageChanged(locale);
    ui->retranslateUi(this);
    ui->generalTab->retranslateUi();
    ui->systemTab->retranslateUi();
    ui->inputTab->retranslateUi();
    ui->graphicsTab->retranslateUi();
    ui->audioTab->retranslateUi();
    ui->cameraTab->retranslateUi();
    ui->debugTab->retranslateUi();
    ui->webTab->retranslateUi();
}
