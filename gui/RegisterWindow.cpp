#include "RegisterWindow.h"

#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ui_RegisterWindow.h"

#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include <exception>
#include <utility>

RegisterWindow::RegisterWindow(std::shared_ptr<ApplicationContext> context,
                               QWidget* parent)
    : QDialog(parent), ui_(std::make_unique<Ui::RegisterWindow>()),
      context_(std::move(context)) {
    ui_->setupUi(this);
    setModal(true);

    ui_->roleComboBox->addItem(QString::fromUtf8(u8"Elev / Student"), "student");
    ui_->roleComboBox->addItem(QString::fromUtf8(u8"Profesor"), "profesor");
    ui_->roleComboBox->setCurrentIndex(-1);

    connect(ui_->registerButton, &QPushButton::clicked,
            this, &RegisterWindow::inregistreaza);
    connect(ui_->backButton, &QPushButton::clicked,
            this, &QDialog::reject);
    connect(ui_->retryConnectionButton, &QPushButton::clicked,
            this, &RegisterWindow::reconecteaza);
    connect(ui_->confirmPasswordLineEdit, &QLineEdit::returnPressed,
            this, &RegisterWindow::inregistreaza);
    actualizeazaStareConexiune();
}

RegisterWindow::~RegisterWindow() = default;

QString RegisterWindow::emailInregistrat() const {
    return emailInregistrat_;
}

bool RegisterWindow::valideazaFormular(const QString& prenume,
                                        const QString& nume,
                                        const QString& email,
                                        const QString& parola,
                                        const QString& confirmare,
                                        const QString& rol) {
    if (prenume.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Prenumele nu poate fi gol."));
        return false;
    }
    if (nume.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Numele nu poate fi gol."));
        return false;
    }
    if (email.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Emailul nu poate fi gol."));
        return false;
    }
    const qsizetype at = email.indexOf('@');
    if (at <= 0 || email.indexOf('.', at + 2) < 0 || email.endsWith('.')) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Adresa de email nu este validă."));
        return false;
    }
    if (parola.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Parola nu poate fi goală."));
        return false;
    }
    if (confirmare.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Confirmarea parolei nu poate fi goală."));
        return false;
    }
    if (parola != confirmare) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Parolele nu coincid."));
        return false;
    }
    if (rol != "student" && rol != "profesor") {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Selectează un rol valid."));
        return false;
    }
    if (!context_->esteConectat()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Nu există conexiune la server."));
        actualizeazaStareConexiune();
        return false;
    }
    return true;
}

void RegisterWindow::inregistreaza() {
    const QString prenume = ui_->firstNameLineEdit->text().trimmed();
    const QString nume = ui_->lastNameLineEdit->text().trimmed();
    const QString email = ui_->emailLineEdit->text().trimmed();
    const QString parola = ui_->passwordLineEdit->text();
    const QString confirmare = ui_->confirmPasswordLineEdit->text();
    const QString rol = ui_->roleComboBox->currentData().toString();

    if (!valideazaFormular(prenume, nume, email, parola, confirmare, rol)) {
        return;
    }

    ui_->registerButton->setEnabled(false);
    ui_->statusLabel->setText(QString::fromUtf8(u8"Creare cont în curs..."));
    try {
        context_->client().inregistreaza(
            nume.toUtf8().toStdString(), prenume.toUtf8().toStdString(),
            email.toUtf8().toStdString(), parola.toUtf8().toStdString(),
            rol.toUtf8().toStdString());
        emailInregistrat_ = email;
        ui_->passwordLineEdit->clear();
        ui_->confirmPasswordLineEdit->clear();
        ui_->statusLabel->setText(
            QString::fromUtf8(u8"Cont creat cu succes. Te poți autentifica."));
        QMessageBox::information(
            this, "EduChain",
            QString::fromUtf8(u8"Cont creat cu succes. Te poți autentifica."));
        accept();
        return;
    } catch (const ExceptieEdu& exceptie) {
        ui_->statusLabel->setText(QString::fromUtf8(exceptie.what()));
    } catch (const std::exception&) {
        ui_->statusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }

    ui_->passwordLineEdit->clear();
    ui_->confirmPasswordLineEdit->clear();
    ui_->registerButton->setEnabled(true);
    try {
        if (context_->esteConectat() && !context_->verificaPing()) {
            context_->deconecteaza();
        }
    } catch (...) {
        context_->deconecteaza();
    }
    actualizeazaStareConexiune();
    if (!context_->esteConectat()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Conexiune pierdută."));
    }
}

void RegisterWindow::reconecteaza() {
    ui_->retryConnectionButton->setEnabled(false);
    ui_->connectionStatusLabel->setText(QString::fromUtf8(u8"Conectare în curs..."));
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

void RegisterWindow::actualizeazaStareConexiune() {
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
