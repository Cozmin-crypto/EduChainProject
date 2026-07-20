#include "LoginWindow.h"

#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "MainWindow.h"
#include "ProtocolEdu.h"
#include "ui_LoginWindow.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

#include <limits>

namespace {
int convertesteIdUtilizator(const std::string& text) {
    try {
        std::size_t convertite = 0;
        const long long valoare = std::stoll(text, &convertite);
        if (convertite != text.size() || valoare <= 0 ||
            valoare > std::numeric_limits<int>::max()) {
            throw ExceptieEdu("Raspunsul autentificarii contine un ID invalid.");
        }
        return static_cast<int>(valoare);
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu("Raspunsul autentificarii contine un ID invalid.");
    }
}
}

LoginWindow::LoginWindow(std::shared_ptr<ApplicationContext> context,
                         QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::LoginWindow>()),
      context_(std::move(context)) {
    ui_->setupUi(this);

    connect(ui_->loginButton, &QPushButton::clicked,
            this, &LoginWindow::autentifica);
    connect(ui_->registerButton, &QPushButton::clicked,
            this, &LoginWindow::afiseazaMesajRegisterNeimplementat);
    connect(ui_->exitButton, &QPushButton::clicked, this, &QWidget::close);
    connect(ui_->passwordLineEdit, &QLineEdit::returnPressed,
            this, &LoginWindow::autentifica);
    connect(ui_->retryConnectionButton, &QPushButton::clicked,
            this, &LoginWindow::reconecteaza);
    actualizeazaStareConexiune();
}

LoginWindow::~LoginWindow() = default;

void LoginWindow::autentifica() {
    const QString email = ui_->emailLineEdit->text().trimmed();
    const QString parola = ui_->passwordLineEdit->text();
    if (email.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Emailul nu poate fi gol."));
        return;
    }
    if (parola.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Parola nu poate fi goală."));
        return;
    }
    if (!context_->esteConectat()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        return;
    }

    ui_->loginButton->setEnabled(false);
    ui_->statusLabel->setText(QString::fromUtf8(u8"Autentificare în curs..."));
    try {
        const std::string emailUtf8 = email.toUtf8().toStdString();
        const auto raspuns = context_->client().autentifica(
            emailUtf8, parola.toUtf8().toStdString());
        if (raspuns.cod != CodRezultatEdu::Succes) {
            ui_->statusLabel->setText(QString::fromUtf8(raspuns.mesajPublic.c_str()));
            ui_->loginButton->setEnabled(true);
            return;
        }
        const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::UtilizatorId);
        const auto rol = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Rol);
        if (!id || !rol || rol->empty()) {
            throw ExceptieEdu("Raspunsul autentificarii este incomplet.");
        }
        context_->salveazaSesiune(convertesteIdUtilizator(*id), emailUtf8, *rol);
        ui_->statusLabel->setText(QString::fromUtf8(u8"Autentificare reușită."));
        auto* mainWindow = new MainWindow(context_);
        mainWindow->setAttribute(Qt::WA_DeleteOnClose);
        mainWindow->show();
        close();
    } catch (const ExceptieEdu& exceptie) {
        ui_->statusLabel->setText(QString::fromUtf8(exceptie.what()));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        actualizeazaStareConexiune();
    }
    ui_->loginButton->setEnabled(true);
}

void LoginWindow::afiseazaMesajRegisterNeimplementat() {
    QMessageBox::information(
        this, "EduChain",
        QString::fromUtf8(u8"Înregistrarea va fi implementată într-un pas ulterior."));
}

void LoginWindow::reconecteaza() {
    ui_->connectionStatusLabel->setText(QString::fromUtf8(u8"Conectare în curs..."));
    ui_->retryConnectionButton->setEnabled(false);
    try {
        context_->reconecteaza();
        ui_->statusLabel->clear();
    } catch (const std::exception&) {
        ui_->statusLabel->setText(
            QString::fromUtf8(u8"Nu s-a putut realiza conexiunea la server."));
    }
    ui_->retryConnectionButton->setEnabled(true);
    actualizeazaStareConexiune();
}

void LoginWindow::actualizeazaStareConexiune() {
    if (context_->esteConectat()) {
        ui_->connectionStatusLabel->setText(
            QString::fromUtf8("Conectat la %1:%2")
                .arg(QString::fromStdString(context_->host()))
                .arg(context_->port()));
        ui_->retryConnectionButton->setVisible(false);
    } else {
        ui_->connectionStatusLabel->setText(QString::fromUtf8(u8"Neconectat"));
        ui_->retryConnectionButton->setVisible(true);
    }
}
