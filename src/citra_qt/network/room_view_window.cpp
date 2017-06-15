// Copyright 2017 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

//#include <functional>
#include <QCloseEvent>
#include <QHeaderView>
#include <QMessageBox>
#include <QScrollBar>
#include <QSplitter>
#include <QStatusBar>
#include "citra_qt/network/room_view_window.h"

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

RoomViewWindow::RoomViewWindow(QWidget* parent) : QMainWindow(parent) {
    InitializeWidgets();
    ConnectWidgetEvents();

    setWindowTitle(tr("Room"));
    show();
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

void RoomViewWindow::closeEvent(QCloseEvent* event) {
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

void RoomViewWindow::InvokeOnStateChanged() {
    QMetaObject::invokeMethod(this, "OnStateChange", Qt::QueuedConnection);
}

void RoomViewWindow::InvokeOnRoomChanged() {
    QMetaObject::invokeMethod(this, "UpdateMemberList", Qt::QueuedConnection);
}

void RoomViewWindow::InvokeOnMessagesReceived() {
    QMetaObject::invokeMethod(this, "OnMessagesReceived", Qt::QueuedConnection);
}

void RoomViewWindow::SetUiState(bool connected) {
    status_bar_label->setText(tr("Not connected"));
    say_line_edit->setEnabled(false);
    item_model->removeRows(0, item_model->rowCount());
}

void RoomViewWindow::OnSay() {
    QString message = say_line_edit->text();
    // TODO(B3N30): Send the chat message
    say_line_edit->setText("");
    say_line_edit->setFocus();
}

void RoomViewWindow::OnConnected() {
    SetUiState(true);
}

void RoomViewWindow::OnDisconnected() {
    SetUiState(false);
}
