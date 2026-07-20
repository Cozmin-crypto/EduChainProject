#include "LoginWindow.h"

#include "ui_LoginWindow.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::LoginWindow>()) {
    ui_->setupUi(this);

    connect(ui_->loginButton, &QPushButton::clicked,
            this, &LoginWindow::afiseazaMesajLoginNeimplementat);
    connect(ui_->registerButton, &QPushButton::clicked,
            this, &LoginWindow::afiseazaMesajRegisterNeimplementat);
    connect(ui_->exitButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui_->passwordLineEdit, &QLineEdit::returnPressed,
            this, &LoginWindow::afiseazaMesajLoginNeimplementat);
}

LoginWindow::~LoginWindow() = default;

void LoginWindow::afiseazaMesajLoginNeimplementat() {
    QMessageBox::information(
        this, "EduChain",
        QString::fromUtf8(u8"Autentificarea va fi implementată în pasul următor."));
}

void LoginWindow::afiseazaMesajRegisterNeimplementat() {
    QMessageBox::information(
        this, "EduChain",
        QString::fromUtf8(u8"Înregistrarea va fi implementată într-un pas ulterior."));
}
