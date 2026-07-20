#include "StudentDashboard.h"
#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ui_StudentDashboard.h"

#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QVariant>

namespace {
QString descrieCurs(const CursPublicEdu& curs) {
    QString text = QString::fromUtf8("%1 (ID: %2, proprietar ID: %3)")
                       .arg(QString::fromUtf8(curs.nume.c_str()))
                       .arg(curs.id).arg(curs.proprietarId);
    if (curs.parinteId) {
        text += QString::fromUtf8(" — părinte ID: %1").arg(*curs.parinteId);
    }
    return text;
}
}

StudentDashboard::StudentDashboard(std::shared_ptr<ApplicationContext> context,
                                   QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::StudentDashboard>()),
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
    connect(ui_->homeButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(0); });
    connect(ui_->myCoursesButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(1); incarcaCursurileMele(); });
    connect(ui_->availableCoursesButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(2); incarcaCursurileDisponibile(); });
    connect(ui_->lessonsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(3); });
    connect(ui_->evaluationsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(4); });
    connect(ui_->resultsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(5); });
    connect(ui_->refreshMyCoursesButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaCursurileMele);
    connect(ui_->refreshAvailableCoursesButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaCursurileDisponibile);
    connect(ui_->enrollButton, &QPushButton::clicked,
            this, &StudentDashboard::inscrieLaCurs);
    connect(ui_->availableCoursesList, &QListWidget::currentItemChanged,
            this, &StudentDashboard::actualizeazaSelectiaCursului);
    ui_->enrollButton->setEnabled(false);
}

StudentDashboard::~StudentDashboard() = default;

bool StudentDashboard::poateExecutaCereri(QLabel* statusLabel) {
    if (!context_->esteAutentificat()) {
        statusLabel->setText(QString::fromUtf8(u8"Sesiunea nu este autentificată."));
        return false;
    }
    if (context_->rol() != "student") {
        statusLabel->setText(QString::fromUtf8(u8"Operația este permisă numai Studentului."));
        return false;
    }
    if (!context_->esteConectat()) {
        statusLabel->setText(QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        ui_->enrollButton->setEnabled(false);
        return false;
    }
    return true;
}

void StudentDashboard::actualizeazaStareConexiune() {
    ui_->connectionLabel->setText(
        context_->esteConectat()
            ? QString::fromUtf8("Conectat la %1:%2")
                  .arg(QString::fromStdString(context_->host())).arg(context_->port())
            : QString::fromUtf8(u8"Conexiune pierdută"));
}

void StudentDashboard::incarcaCursurileMele() {
    if (!poateExecutaCereri(ui_->myCoursesStatusLabel)) return;
    ui_->refreshMyCoursesButton->setEnabled(false);
    ui_->myCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare în curs..."));
    try {
        const auto cursuri = context_->client().listeazaCursuriInscrise();
        ui_->myCoursesList->clear();
        for (const auto& curs : cursuri) {
            auto* element = new QListWidgetItem(descrieCurs(curs), ui_->myCoursesList);
            element->setData(Qt::UserRole, curs.id);
        }
        ui_->myCoursesStatusLabel->setText(
            cursuri.empty() ? QString::fromUtf8(u8"Nu ești înscris la niciun curs.")
                            : QString());
    } catch (const ExceptieEdu& eroare) {
        ui_->myCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
    } catch (const std::exception&) {
        ui_->myCoursesStatusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    ui_->refreshMyCoursesButton->setEnabled(context_->esteConectat());
    actualizeazaStareConexiune();
}

void StudentDashboard::incarcaCursurileDisponibile() {
    if (!poateExecutaCereri(ui_->availableCoursesStatusLabel)) return;
    ui_->refreshAvailableCoursesButton->setEnabled(false);
    ui_->enrollButton->setEnabled(false);
    ui_->availableCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare în curs..."));
    try {
        const auto cursuri = context_->client().listeazaCursuriDisponibile();
        ui_->availableCoursesList->clear();
        for (const auto& curs : cursuri) {
            auto* element = new QListWidgetItem(descrieCurs(curs), ui_->availableCoursesList);
            element->setData(Qt::UserRole, curs.id);
        }
        ui_->availableCoursesStatusLabel->setText(
            cursuri.empty()
                ? QString::fromUtf8(u8"Nu există cursuri disponibile pentru înscriere.")
                : QString());
    } catch (const ExceptieEdu& eroare) {
        ui_->availableCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
    } catch (const std::exception&) {
        ui_->availableCoursesStatusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    ui_->refreshAvailableCoursesButton->setEnabled(context_->esteConectat());
    actualizeazaSelectiaCursului();
    actualizeazaStareConexiune();
}

void StudentDashboard::actualizeazaSelectiaCursului() {
    const auto* element = ui_->availableCoursesList->currentItem();
    const bool selectieValida = element && element->data(Qt::UserRole).toInt() > 0;
    ui_->enrollButton->setEnabled(selectieValida && context_->esteConectat() &&
                                  context_->esteAutentificat() && context_->rol() == "student");
}

void StudentDashboard::inscrieLaCurs() {
    if (!poateExecutaCereri(ui_->availableCoursesStatusLabel)) return;
    auto* element = ui_->availableCoursesList->currentItem();
    const int cursId = element ? element->data(Qt::UserRole).toInt() : 0;
    if (cursId <= 0) {
        ui_->availableCoursesStatusLabel->setText(QString::fromUtf8(u8"Selectează un curs valid."));
        ui_->enrollButton->setEnabled(false);
        return;
    }
    ui_->enrollButton->setEnabled(false);
    ui_->availableCoursesStatusLabel->setText(QString::fromUtf8(u8"Înscriere în curs..."));
    try {
        context_->client().inscrieLaCurs(cursId);
        incarcaCursurileMele();
        incarcaCursurileDisponibile();
        ui_->availableCoursesStatusLabel->setText(
            QString::fromUtf8(u8"Înscriere realizată cu succes."));
    } catch (const ExceptieEdu& eroare) {
        const QString mesaj = context_->esteConectat()
            ? QString::fromUtf8(eroare.what())
            : QString::fromUtf8(u8"Conexiune pierdută.");
        if (context_->esteConectat()) {
            incarcaCursurileMele();
            incarcaCursurileDisponibile();
        }
        ui_->availableCoursesStatusLabel->setText(mesaj);
    } catch (const std::exception&) {
        ui_->availableCoursesStatusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    actualizeazaSelectiaCursului();
    actualizeazaStareConexiune();
}
