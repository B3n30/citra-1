// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QStandardItemModel>
#include <QTextEdit>
#include <QTimer>
#include <QTreeView>
#include <QVBoxLayout>

class RoomViewWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit RoomViewWindow(QWidget* parent = 0);
    ~RoomViewWindow();

private:
    void InitializeWidgets();
    bool ConfirmLeaveRoom();
    bool ConfirmCloseRoom();
    void closeEvent(QCloseEvent* event);
    void ConnectWidgetEvents();
    void AddConnectionMessage(QString message);
    void SetUiState(bool connected);

private slots:
    void OnSay();
    void OnConnected();
    void OnDisconnected();
    void InvokeOnStateChanged();
    void InvokeOnRoomChanged();
    void InvokeOnMessagesReceived();

private:
    QWidget* chat_widget = nullptr;
    QGridLayout* grid_layout = nullptr;
    QTextEdit* chat_log = nullptr;
    QTreeView* member_list = nullptr;
    QLineEdit* say_line_edit = nullptr;
    QPushButton* say_button = nullptr;
    QStandardItemModel* item_model = nullptr;
    QLabel* status_bar_label = nullptr;
};
