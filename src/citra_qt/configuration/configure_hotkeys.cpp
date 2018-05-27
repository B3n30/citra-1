// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <QKeyEvent>
#include <QMessageBox>
#include <QTimer>
#include "citra_qt/configuration/configure_hotkeys.h"
#include "citra_qt/hotkeys.h"
#include "citra_qt/ui_settings.h"
#include "citra_qt/util/sequence_dialog/sequence_dialog.h"
#include "common/param_package.h"
#include "core/settings.h"
#include "input_common/main.h"
#include "ui_configure_hotkeys.h"

ConfigureHotkeys::ConfigureHotkeys(QWidget* parent)
    : QWidget(parent), ui(std::make_unique<Ui::ConfigureHotkeys>()) {
    ui->setupUi(this);
    setFocusPolicy(Qt::ClickFocus);

    QStandardItemModel* model = new QStandardItemModel(this);
    model->setColumnCount(3);
    model->setHorizontalHeaderLabels({tr("Action"), tr("Hotkey"), tr("Context")});

    for (auto group : hotkey_groups) {
        QStandardItem* parent_item = new QStandardItem(group.first);
        parent_item->setEditable(false);
        for (auto hotkey : group.second) {
            QStandardItem* action = new QStandardItem(hotkey.first);
            QStandardItem* keyseq = new QStandardItem(hotkey.second.keyseq.toString());
            action->setEditable(false);
            keyseq->setEditable(false);
            parent_item->appendRow({action, keyseq});
        }
        model->appendRow(parent_item);
    }

    ui->hotkey_list->setSelectionMode(QTreeView::SingleSelection);
    ui->hotkey_list->setModel(model);

    ui->hotkey_list->expandAll();

    // TODO: Make context configurable as well (hiding the column for now)
    ui->hotkey_list->hideColumn(2);

    ui->hotkey_list->setColumnWidth(0, 200);
    ui->hotkey_list->resizeColumnToContents(1);

    ui->hotkey_list->installEventFilter(this);

    std::transform(Settings::values.buttons.begin(), Settings::values.buttons.end(),
                   buttons_param.begin(),
                   [](const std::string& str) { return Common::ParamPackage(str); });
    std::transform(Settings::values.analogs.begin(), Settings::values.analogs.end(),
                   analogs_param.begin(),
                   [](const std::string& str) { return Common::ParamPackage(str); });
}

ConfigureHotkeys::~ConfigureHotkeys() {}

bool ConfigureHotkeys::eventFilter(QObject* o, QEvent* e) {
    if (o == ui->hotkey_list && e->type() != QEvent::ContextMenu) {
        return false;
    }

    for (QModelIndex index : ui->hotkey_list->selectionModel()->selectedIndexes()) {
        if (index.row() == -1 || index.column() != 1) {
            continue;
        } else {
            configure(index);
        }
    }

    return false;
}

void ConfigureHotkeys::configure(QModelIndex index) {

    auto model = ui->hotkey_list->model();
    auto previous_key = model->data(index);

    SequenceDialog* hotkey_dialog = new SequenceDialog();
    hotkey_dialog->setWindowTitle(QString::fromStdString("Enter a hotkey"));
    hotkey_dialog->exec();

    auto key_string = hotkey_dialog->getSequence();

    if (isUsedKey(key_string)) {
        model->setData(index, previous_key);
        QMessageBox::critical(this, tr("Error!"), tr("You're using a key that's already bound."));
    } else if (key_string == QKeySequence(Qt::Key_Escape)) {
        model->setData(index, previous_key);
    } else {
        model->setData(index, key_string.toString());
    }
}

const std::array<std::string, ConfigureHotkeys::ANALOG_SUB_BUTTONS_NUM>
    ConfigureHotkeys::analog_sub_buttons{{
        "up",
        "down",
        "left",
        "right",
        "modifier",
    }};

static QString getKeyName(int key_code) {
    switch (key_code) {
    case Qt::Key_Shift:
        return QObject::tr("Shift");
    case Qt::Key_Control:
        return QObject::tr("Ctrl");
    case Qt::Key_Alt:
        return QObject::tr("Alt");
    case Qt::Key_Meta:
        return "";
    default:
        return QKeySequence(key_code).toString();
    }
}

bool ConfigureHotkeys::isUsedKey(QKeySequence key_sequence) {

    // Check HotKeys
    auto model = static_cast<QStandardItemModel*>(ui->hotkey_list->model());
    for (int r = 0; r < model->rowCount(); r++) {
        QStandardItem* parent = model->item(r, 0);
        for (int r2 = 0; r2 < parent->rowCount(); r2++) {
            QStandardItem* keyseq = parent->child(r2, 1);
            if (keyseq->text() == key_sequence.toString()) {
                return true;
            }
        }
    }

    // Check InputKeys
    const QString non_keyboard(tr("[non-keyboard]"));

    auto KeyToText = [&non_keyboard](const Common::ParamPackage& param) {
        if (param.Get("engine", "") != "keyboard") {
            return non_keyboard;
        } else {
            return getKeyName(param.Get("code", 0));
        }
    };

    for (int button = 0; button < Settings::NativeButton::NumButtons; button++) {
        if (KeyToText(buttons_param[button]) == key_sequence.toString()) {
            return true;
        }
    }

    for (int analog_id = 0; analog_id < Settings::NativeAnalog::NumAnalogs; analog_id++) {
        if (analogs_param[analog_id].Get("engine", "") != "analog_from_button") {
            return false;
        } else {
            for (int sub_button_id = 0; sub_button_id < ANALOG_SUB_BUTTONS_NUM; sub_button_id++) {
                Common::ParamPackage param(
                    analogs_param[analog_id].Get(analog_sub_buttons[sub_button_id], ""));
                if (KeyToText(param) == key_sequence.toString()) {
                    return true;
                }
            }
        }
    }

    return false;
}

void ConfigureHotkeys::applyConfiguration() {
    auto model = static_cast<QStandardItemModel*>(ui->hotkey_list->model());

    for (int key_id = 0; key_id < model->rowCount(); key_id++) {
        QStandardItem* parent = model->item(key_id, 0);
        for (int key_column_id = 0; key_column_id < parent->rowCount(); key_column_id++) {
            QStandardItem* action = parent->child(key_column_id, 0);
            QStandardItem* keyseq = parent->child(key_column_id, 1);
            for (HotkeyGroupMap::iterator key_iterator = hotkey_groups.begin();
                 key_iterator != hotkey_groups.end(); ++key_iterator) {
                if (key_iterator->first == parent->text()) {
                    for (HotkeyMap::iterator it2 = key_iterator->second.begin();
                         it2 != key_iterator->second.end(); ++it2) {
                        if (it2->first == action->text()) {
                            it2->second.keyseq = QKeySequence(keyseq->text());
                        }
                    }
                }
            }
        }
    }

    SaveHotkeys();
    Settings::Apply();
}
