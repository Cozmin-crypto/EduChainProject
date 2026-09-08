#include "LoginWindow.h"

#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "MainWindow.h"
#include "ProtocolEdu.h"
#include "RegisterWindow.h"
#include "ui_LoginWindow.h"

#include <QLineEdit>
#include <QPushButton>
#include <QScreen>
#include <QString>
#include <QTimer>

#include <limits>

namespace {
int convertesteIdUtilizator(const std::string& text) {
    try {
        std::size_t convertite = 0;
        const long long valoare = std::stoll(text, &convertite);
        if (convertite != text.size() || valoare <= 0 || valoare > std::numeric_limits<int>::max()) 
        {
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

LoginWindow::LoginWindow(std::shared_ptr<ApplicationContext> context, QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::LoginWindow>()), context_(std::move(context)) 
{
    ui_->setupUi(this);
    //this se refera la fereastra curenta,

    //iar ui_ este un pointer inteligent care gestioneaza obiectul Ui::LoginWindow


    //setupUi(this) initializeaza interfata grafica a ferestrei curente folosind 
    //obiectul Ui::LoginWindow, care a fost generat de Qt Designer pe baza
    //fisierului .ui asociat ferestrei de autentificare

    connect(ui_->loginButton, &QPushButton::clicked, this, &LoginWindow::autentifica);
        //Conectarea butonului de autentificare la slotul de autentificare

    connect(ui_->registerButton, &QPushButton::clicked, this, &LoginWindow::deschideInregistrare);
        //Conectarea butonului de inregistrare la slotul de deschidere a 
        //ferestrei de inregistrare a utilizatorului
    
    connect(ui_->exitButton, &QPushButton::clicked, this, &QWidget::close);
        //Conectarea butonului de iesire la slotul de inchidere a ferestrei 
    
    connect(ui_->passwordLineEdit, &QLineEdit::returnPressed, this, &LoginWindow::autentifica);
        //Conectarea apasarii tastei Enter in campul de parola la slotul de autentificare 
    
    connect(ui_->retryConnectionButton, &QPushButton::clicked, this, &LoginWindow::reconecteaza);
        //conectarea butonului de reconectare la slotul de reconectare la server 
    
    actualizeazaStareConexiune();
    //Dupa fiecare actiune, se actualizeaza starea conexiunii la server 
    //pentru a reflecta starea curenta a conexiunii in interfata grafica
}

LoginWindow::~LoginWindow() = default;

void LoginWindow::autentifica() {
    if (autentificareInCurs_) return;
    const QString email = ui_->emailLineEdit->text().trimmed();
    const QString parola = ui_->passwordLineEdit->text();


    //se verifica daca campurile email si parola sunt goale, iar daca da, se afiseaza un mesaj de eroare
    if (email.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Emailul nu poate fi gol."));
        return;
    }
    if (parola.isEmpty()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Parola nu poate fi goală."));
        return;
    }




    //verificam daca aplicatia mai are conexiune cu serverul
    if (!context_->esteConectat()) {
        ui_->statusLabel->setText(QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        return;
    }

    //dupa ce s a apasat o data butonul de autentificare, acesta este dezactivat, 
    //pentru a evita dublu click si 2 cereri de logare consecutive
    autentificareInCurs_ = true;
    actualizeazaStareConexiune();
    ui_->statusLabel->setText(QString::fromUtf8(u8"Autentificare în curs..."));




    try {
        //convertim email ul din Qstring in std::string pentru a fi folosit
        //in cererea de autentificare catre server 
        const std::string emailUtf8 = email.toUtf8().toStdString();
        const std::string parolaUtf8 = parola.toUtf8().toStdString();
        //de ce? pentru ca GUI-ul lucreaza cu Qstring, 
        //iar protocolul de comunicatie cu serverul lucreaza cu std::string 

        const auto raspuns = context_->client().autentifica(emailUtf8, parolaUtf8);
        
        
        if (raspuns.cod != CodRezultatEdu::Succes) {
            ui_->statusLabel->setText(QString::fromUtf8(raspuns.mesajPublic.c_str()));
            autentificareInCurs_ = false;
            actualizeazaStareConexiune();
            return;
        
        
        }
        const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::UtilizatorId);
        const auto rol = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Rol);
        const auto nume = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Nume);
        const auto prenume = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Prenume);
        //luam rolul si id ul utilizatorului din raspunsul serverului
        //pentru a le salva in sesiunea curenta a aplicatiei, pentru a fi folosite ulterior


        if (!id || !rol || rol->empty() || !nume || nume->empty() ||
            !prenume || prenume->empty()) {
            throw ExceptieEdu("Raspunsul autentificarii este incomplet.");
        }



        context_->salveazaSesiune(convertesteIdUtilizator(*id), emailUtf8, *rol,
                                  *nume, *prenume);
        //converteste id ul din string in int 

        //salveaza datele utilizatorului in sesiunea curenta pentru a fi folosite ulterior 


        ui_->statusLabel->setText(QString::fromUtf8(u8"Autentificare reușită."));
        //daca autentificarea a fost reusita, se deschide fereastra principala a aplicatiei


        auto* mainWindow = new MainWindow(context_);
        //cream fereastra principala si ii transmitem acelasi context
        //pentru a putea folosi acelasi client si datele de sesiune salvate in context 
        //este acelasi shared::ptr

        mainWindow->setAttribute(Qt::WA_DeleteOnClose);
        //cerem Qt ului sa distruga automat obiectul MainWindow cand fereastra este inchisa 

        mainWindow->show();
        close();
        //se arata fereastra principala, iar fereastra de login se inchide 
    } 
    catch (const ExceptieEdu& exceptie) 
    {
        ui_->statusLabel->setText(QString::fromUtf8(exceptie.what()));
        actualizeazaStareConexiune();
    } 
    catch (const std::exception&) 
    {
        ui_->statusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        actualizeazaStareConexiune();
    }
    autentificareInCurs_ = false;
    actualizeazaStareConexiune();
}

void LoginWindow::deschideInregistrare() {
    if (!context_->esteConectat()) {
        actualizeazaStareConexiune();
        return;
    }
    ui_->registerButton->setEnabled(false);
    RegisterWindow fereastra(context_, this);
    QTimer::singleShot(0, &fereastra, [&fereastra] {
        fereastra.adjustSize();
        if (const QScreen* ecran = fereastra.screen()) {
            fereastra.move(ecran->availableGeometry().center() -
                           fereastra.rect().center());
        }
    });
    if (fereastra.exec() == QDialog::Accepted) {
        ui_->emailLineEdit->setText(fereastra.emailInregistrat());
        ui_->passwordLineEdit->clear();
        ui_->passwordLineEdit->setFocus();
        ui_->statusLabel->setText(
            QString::fromUtf8(u8"Cont creat cu succes. Te poți autentifica."));
    }
    ui_->registerButton->setEnabled(true);
    actualizeazaStareConexiune();
}

void LoginWindow::reconecteaza() {
    if (reconectareInCurs_) return;
    reconectareInCurs_ = true;
    ui_->statusLabel->setText(QString::fromUtf8(u8"Se încearcă reconectarea..."));
    actualizeazaStareConexiune();
    try {
        context_->reconecteaza();
        ui_->statusLabel->setText(
            QString::fromUtf8("Conectat la %1:%2")
                .arg(QString::fromStdString(context_->host()))
                .arg(context_->port()));
    } catch (const std::exception&) {
        context_->deconecteaza();
    }
    reconectareInCurs_ = false;
    actualizeazaStareConexiune();
}

void LoginWindow::actualizeazaStareConexiune() {
    const bool conectat = context_->esteConectat();
    ui_->loginButton->setEnabled(conectat && !autentificareInCurs_ && !reconectareInCurs_);
    ui_->registerButton->setEnabled(conectat && !autentificareInCurs_ && !reconectareInCurs_);
    ui_->retryConnectionButton->setVisible(true);
    ui_->retryConnectionButton->setEnabled(!conectat && !reconectareInCurs_);
    ui_->exitButton->setEnabled(true);
    if (conectat) {
        ui_->connectionStatusLabel->setText(
            QString::fromUtf8("Conectat la %1:%2")
                .arg(QString::fromStdString(context_->host()))
                .arg(context_->port()));
    } else {
        ui_->connectionStatusLabel->setText(QString::fromUtf8(u8"Server indisponibil"));
        if (!reconectareInCurs_) {
            const auto& eroare = context_->ultimaEroareConexiune();
            ui_->statusLabel->setText(eroare.empty()
                ? QString::fromUtf8(u8"Nu există conexiune.")
                : QString::fromUtf8(eroare.c_str()));
        }
    }
}
