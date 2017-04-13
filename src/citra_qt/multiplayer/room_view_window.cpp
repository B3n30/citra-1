// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <deque>
#include <functional>
#include <QCloseEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>

#include "citra_qt/multiplayer/room_view_window.h"
#include "common/logging/log.h"

static void AppendHtml(QTextEdit* text_edit, QString html) {
    auto* scrollbar = text_edit->verticalScrollBar();
    auto scrollbar_value = scrollbar->value();
    bool scroll_down = (scrollbar_value == scrollbar->maximum());

    QString new_html = text_edit->toHtml();
    new_html += html;
    text_edit->setHtml(new_html);

    // Scroll to the very bottom
    if (scroll_down) {
      scrollbar->setValue(scrollbar->maximum());
    } else {
      scrollbar->setValue(scrollbar_value);
    }
}

RoomViewWindow::RoomViewWindow(QWidget *parent) : QMainWindow(parent) {
    InitializeWidgets();
    ConnectWidgetEvents();

    room = NetCore::getRoom();
    room_member = NetCore::getRoomMember();

    setWindowTitle(tr("Room"));
    show();
    
    ConnectRoomEvents();
    OnStateChange();
    UpdateMemberList();
}

RoomViewWindow::~RoomViewWindow() {}

void RoomViewWindow::InitializeWidgets() {
    QWidget* central_widget = new QWidget();
    QVBoxLayout* vertical_layout = new QVBoxLayout();
    central_widget->setLayout(vertical_layout);

    QSplitter* splitter = new QSplitter();

    QWidget* splitter_left = new QWidget();
    QVBoxLayout* splitter_left_layout = new QVBoxLayout();
    splitter_left->setLayout(splitter_left_layout);
    splitter->insertWidget(0, splitter_left);

    QWidget* splitter_right = new QWidget();
    QVBoxLayout* splitter_right_layout = new QVBoxLayout();
    splitter_right->setLayout(splitter_right_layout);
    splitter->insertWidget(1, splitter_right);

    chat_widget = new QWidget();
    grid_layout = new QGridLayout();
    chat_widget->setLayout(grid_layout);

    chat_log = new QTextEdit();
    chat_log->setReadOnly(true);
    splitter_left_layout->addWidget(chat_log);

    member_list = new QTreeView();
    item_model = new QStandardItemModel(member_list);
    member_list->setModel(item_model);
    member_list->setAlternatingRowColors(true);
    member_list->setSelectionMode(QHeaderView::SingleSelection);
    member_list->setSelectionBehavior(QHeaderView::SelectRows);
    member_list->setVerticalScrollMode(QHeaderView::ScrollPerPixel);
    member_list->setHorizontalScrollMode(QHeaderView::ScrollPerPixel);
    member_list->setSortingEnabled(true);
    member_list->setEditTriggers(QHeaderView::NoEditTriggers);
    member_list->setUniformRowHeights(true);
    member_list->setContextMenuPolicy(Qt::CustomContextMenu);

    enum {
        COLUMN_NAME,
        COLUMN_GAME,
        COLUMN_MAC_ADDRESS,
        COLUMN_ACTIVITY,
        COLUMN_PING,
        COLUMN_COUNT, // Number of columns
    };

    item_model->insertColumns(0, COLUMN_COUNT);
    item_model->setHeaderData(COLUMN_NAME, Qt::Horizontal, tr("Name"));
    item_model->setHeaderData(COLUMN_GAME, Qt::Horizontal, tr("Game"));
    item_model->setHeaderData(COLUMN_MAC_ADDRESS, Qt::Horizontal, tr("MAC Address"));
    item_model->setHeaderData(COLUMN_ACTIVITY, Qt::Horizontal, tr("Activity"));
    item_model->setHeaderData(COLUMN_PING, Qt::Horizontal, tr("Ping"));

    member_list->header()->setStretchLastSection(false);
    member_list->header()->setSectionResizeMode(COLUMN_GAME, QHeaderView::Stretch);
    splitter_right_layout->addWidget(member_list);

    status_bar_label = new QLabel();
    status_bar_label->setVisible(true);
    status_bar_label->setFrameStyle(QFrame::NoFrame);
    status_bar_label->setContentsMargins(4, 0, 4, 0);
    status_bar_label->setText(tr("Not connected"));
    statusBar()->addPermanentWidget(status_bar_label);
    statusBar()->setVisible(true);

    say_line_edit = new QLineEdit(); // <item row="1" column="0" colspan="2" >
    splitter_left_layout->addWidget(say_line_edit);

    say_button = new QPushButton(tr("Say")); // <item row="1" column="2" >
    say_button->show();
    splitter_right_layout->addWidget(say_button);

    vertical_layout->addWidget(splitter);

    setCentralWidget(central_widget);

}

bool RoomViewWindow::ConfirmLeaveRoom() {
    auto answer = QMessageBox::question(
        this, tr("Citra"),
        tr("Are you sure you want to leave this room? Your simulated WiFi connection to all other members will be lost."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

bool RoomViewWindow::ConfirmCloseRoom() {
    auto answer = QMessageBox::question(
        this, tr("Citra"),
        tr("Are you sure you want to close this room? The simulated WiFi connections of all members will be lost."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    return answer != QMessageBox::No;
}

void RoomViewWindow::closeEvent(QCloseEvent* event) {
    if (room->GetState() != Room::State::Closed) {
        if (!ConfirmCloseRoom()) {
            event->ignore();
            return;
        }
        room_member->Leave();
        room->Destroy();
    } else if (room_member->IsConnected()) {
        if (!ConfirmLeaveRoom()) {
            event->ignore();
            return;
        }
        room_member->Leave();
    }
    QWidget::closeEvent(event);
}

void RoomViewWindow::ConnectWidgetEvents() {
    connect(say_line_edit, SIGNAL(returnPressed()), say_button, SIGNAL(clicked()));
    connect(say_button, SIGNAL(clicked()), this, SLOT(OnSay()));
}

void RoomViewWindow::AddConnectionMessage(QString message) {
    QString html;
    html += "<font color=\"green\"><b>" + message.toHtmlEscaped() + "</b></font><br>";
    AppendHtml(chat_log, html);
}

void RoomViewWindow::ConnectRoomEvents() {
    room_member->Connect(std::bind(&RoomViewWindow::InvokeOnStateChanged,this), RoomMember::EventType::OnStateChanged);
    room_member->Connect(std::bind(&RoomViewWindow::InvokeOnRoomChanged,this), RoomMember::EventType::OnRoomChanged);
    room_member->Connect(std::bind(&::RoomViewWindow::InvokeOnMessagesReceived,this), RoomMember::EventType::OnMessagesReceived);
}

void RoomViewWindow::InvokeOnStateChanged() {
    QMetaObject::invokeMethod(this, "OnStateChange", Qt::QueuedConnection);
}

void RoomViewWindow::InvokeOnRoomChanged() {
    QMetaObject::invokeMethod(this, "UpdateMemberList", Qt::QueuedConnection);
}

void RoomViewWindow::InvokeOnMessagesReceived() {
    QMetaObject::invokeMethod(this, "OnMessagesReceived", Qt::QueuedConnection);
}

void RoomViewWindow::UpdateMemberList() {
    //TODO(B3N30): Remember which row is selected

    auto member_list = room_member->GetMemberInformation();

    auto MacAddressString = [&](const MacAddress& mac_address) -> QString {
        QString str;
        for (unsigned int i = 0; i < mac_address.size(); i++) {
            if (i > 0) {
                str += ":";
            }
            str += QString("%1").arg(mac_address[i], 2, 16, QLatin1Char('0')).toUpper();
        }
        return str;
    };

    item_model->removeRows(0, item_model->rowCount());
    for (const auto& member : member_list) {
        QList<QStandardItem*> l;

        std::vector<std::string> elements = {
            member.nickname,
            member.game_name,
            MacAddressString(member.mac_address).toStdString(),
            "- %",
            "- ms"
        };
        for (auto& item : elements) {
                QStandardItem *child = new QStandardItem(QString::fromStdString(item));
                child->setEditable( false );
            l.append(child);
        }
        item_model->invisibleRootItem()->appendRow(l);
    }

    //TODO(B3N30): Restore row selection
}

void RoomViewWindow::SetUiState(bool connected) {
    if (connected) {
        status_bar_label->setText(tr("Connected to: %1").arg(room_member->GetRoomInformation().name.c_str()));
        say_line_edit->setEnabled(true);
    } else {
        status_bar_label->setText(tr("Not connected"));
        say_line_edit->setEnabled(false);
        item_model->removeRows(0, item_model->rowCount());
    }
}


void RoomViewWindow::OnSay() {
    QString message = say_line_edit->text();
    room_member->SendChatMessage(message.toStdString());
    say_line_edit->setText("");
    say_line_edit->setFocus();
}

void RoomViewWindow::OnMessagesReceived() {
    std::deque<RoomMember::ChatEntry> entries(room_member->PopChatEntries());
    for (auto entry : entries) {
        QString html;
        if (entry.nickname == room_member->GetNickname())
            html += "<font color=\"Red\"><b>" + QString::fromStdString(entry.nickname).toHtmlEscaped() + ":</b></font> ";
        else
            html += "<font color=\"RoyalBlue\"><b>" + QString::fromStdString(entry.nickname).toHtmlEscaped() + ":</b></font> ";
        html += QString::fromStdString(entry.message).toHtmlEscaped();
        AppendHtml(chat_log, html);
    }
}

void RoomViewWindow::OnStateChange() {
    switch(room_member->GetState()) {
    case RoomMember::State::Idle:
        break;
    case RoomMember::State::Error:
        AddConnectionMessage(tr("The network could not be used. Make sure your system is connected to the network and you have the necessary permissions"));
        break;
    case RoomMember::State::Joining:
        chat_log->setHtml(""); //FIXME: Only do this when the server has changed, not in case of reconnect attempts!
        AddConnectionMessage(tr("Attempting to join room (Connecting)..."));
        break;
    case RoomMember::State::Joined:
        AddConnectionMessage(tr("Room joined successfully (Connected)"));
        OnConnected();
        break;
    case RoomMember::State::LostConnection:
        AddConnectionMessage(tr("Disconnected (Lost connection to room)"));
        OnDisconnected();
        break;
    case RoomMember::State::RoomFull:
        AddConnectionMessage(tr("Unable to join (The room is full)"));
        break;
    case RoomMember::State::RoomDestroyed:
        AddConnectionMessage(tr("Unable to join (The room could not be found)"));
        break;
    case RoomMember::State::NameCollision:
        AddConnectionMessage(tr("Unable to join (The nickname is already in use)"));
        break;
    case RoomMember::State::MacCollision:
        AddConnectionMessage(tr("Unable to join (The preferred mac address is already in use)"));
        break;
    case RoomMember::State::WrongVersion:
        AddConnectionMessage(tr("Unable to join (Room is using another Citra version)"));
        break;
    default:
        assert(false);
        break;
    }
}

void RoomViewWindow::OnConnected() {
    SetUiState(true);
}

void RoomViewWindow::OnDisconnected() {
    SetUiState(false);
}
