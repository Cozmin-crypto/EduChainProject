#include "MainWindow.h"

#include "ApplicationContext.h"
#include "LoginWindow.h"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

MainWindow::MainWindow(std::shared_ptr<ApplicationContext> context, QWidget* parent)
    : QMainWindow(parent), ui_(std::make_unique<Ui::MainWindow>()),
      context_(std::move(context)) {
    ui_->setupUi(this);
    ui_->userLabel->setText(QString::fromUtf8("Utilizator: %1")
                                .arg(QString::fromStdString(context_->email())));
    ui_->roleLabel->setText(QString::fromUtf8("Rol: %1")
                                .arg(QString::fromStdString(context_->rol())));
    ui_->connectionLabel->setText(
        context_->esteConectat() ? QString::fromUtf8(u8"Conectat")
                                 : QString::fromUtf8(u8"Conexiune pierdută"));
    connect(ui_->logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    connect(ui_->closeButton, &QPushButton::clicked,
            this, &MainWindow::inchideAplicatia);
}

MainWindow::~MainWindow() = default;

void MainWindow::logout() {
    context_->deconecteaza();
    try {
        context_->conecteaza();
    } catch (...) {
    }
    auto* loginWindow = new LoginWindow(context_);
    loginWindow->setAttribute(Qt::WA_DeleteOnClose);
    loginWindow->show();
    close();
}

void MainWindow::inchideAplicatia() {
    context_->deconecteaza();
    QApplication::quit();
}
