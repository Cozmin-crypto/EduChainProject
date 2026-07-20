#include "MainWindow.h"

#include "AdminDashboard.h"
#include "ApplicationContext.h"
#include "LoginWindow.h"
#include "ProfessorDashboard.h"
#include "StudentDashboard.h"
#include "ui_MainWindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QTimer>

MainWindow::MainWindow(std::shared_ptr<ApplicationContext> context, QWidget* parent)
    : QMainWindow(parent), ui_(std::make_unique<Ui::MainWindow>()),
      context_(std::move(context)) {
    ui_->setupUi(this);
    ui_->userLabel->setText(QString::fromUtf8("Utilizator: %1")
                                .arg(QString::fromStdString(context_->email())));
    ui_->roleLabel->setText(QString::fromUtf8("Rol: %1")
                                .arg(QString::fromStdString(context_->rol())));
    ui_->connectionLabel->setText(
        context_->esteConectat()
            ? QString::fromUtf8("Conectat la %1:%2")
                  .arg(QString::fromStdString(context_->host())).arg(context_->port())
            : QString::fromUtf8(u8"Conexiune pierdută"));
    connect(ui_->logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    connect(ui_->closeButton, &QPushButton::clicked,
            this, &MainWindow::inchideAplicatia);
    configureazaDashboard();
}

MainWindow::~MainWindow() = default;

void MainWindow::logout() {
    ui_->logoutButton->setEnabled(false);
    context_->deconecteaza();
    if (!reconecteazaCuDialog()) {
        QApplication::quit();
        return;
    }
    revinoLaLogin();
}

void MainWindow::revinoLaLogin() {
    auto* loginWindow = new LoginWindow(context_);
    loginWindow->setAttribute(Qt::WA_DeleteOnClose);
    loginWindow->show();
    inchiderePentruSchimbareFereastra_ = true;
    close();
}

void MainWindow::inchideAplicatia() {
    close();
}

void MainWindow::configureazaDashboard() {
    const std::string& rol = context_->rol();
    QWidget* dashboard = nullptr;
    if (rol == "student") {
        dashboard = new StudentDashboard(context_, ui_->dashboardStack);
    } else if (rol == "profesor") {
        dashboard = new ProfessorDashboard(context_, ui_->dashboardStack);
    } else if (rol == "administrator") {
        dashboard = new AdminDashboard(context_, ui_->dashboardStack);
    } else {
        QTimer::singleShot(0, this, &MainWindow::trateazaRolNecunoscut);
        return;
    }
    ui_->dashboardStack->addWidget(dashboard);
    ui_->dashboardStack->setCurrentWidget(dashboard);
}

bool MainWindow::reconecteazaCuDialog() {
    while (!context_->esteConectat()) {
        try {
            context_->conecteaza();
            return true;
        } catch (...) {
            QMessageBox mesaj(this);
            mesaj.setIcon(QMessageBox::Critical);
            mesaj.setWindowTitle("EduChain");
            mesaj.setText(QString::fromUtf8(
                u8"Nu s-a putut realiza conexiunea la server."));
            auto* retry = mesaj.addButton("Retry", QMessageBox::AcceptRole);
            mesaj.addButton("Exit", QMessageBox::RejectRole);
            mesaj.exec();
            if (mesaj.clickedButton() != retry) {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::trateazaRolNecunoscut() {
    QMessageBox::critical(this, "EduChain",
                          QString::fromUtf8(u8"Rolul utilizatorului nu este recunoscut."));
    context_->deconecteaza();
    if (reconecteazaCuDialog()) {
        revinoLaLogin();
    } else {
        QApplication::quit();
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!inchiderePentruSchimbareFereastra_) {
        context_->deconecteaza();
        QApplication::quit();
    }
    event->accept();
}
