#include "StudentDashboard.h"
#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ui_StudentDashboard.h"

#include <QComboBox>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QVariant>

#include <algorithm>

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
            [this] {
                ui_->pagesStack->setCurrentIndex(3);
                incarcaCursurilePentruLectii();
            });
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
    connect(ui_->refreshLessonCoursesButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaCursurilePentruLectii);
    connect(ui_->refreshLessonsButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaLectiileCursului);
    connect(ui_->lessonCourseCombo,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) { incarcaLectiileCursului(); });
    connect(ui_->lessonsList, &QListWidget::currentItemChanged,
            this, &StudentDashboard::afiseazaLectiaSelectata);
    ui_->enrollButton->setEnabled(false);
    golesteDetaliileLectiei();
    actualizeazaControaleleLectiilor();
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

void StudentDashboard::golesteDetaliileLectiei() {
    ui_->lessonTitleLabel->setText(QString::fromUtf8(u8"Nicio lecție selectată"));
    ui_->lessonMetadataLabel->clear();
    ui_->lessonContent->clear();
}

void StudentDashboard::actualizeazaControaleleLectiilor(bool cerereInCurs) {
    const bool disponibil = context_->esteConectat() && context_->esteAutentificat() &&
                            context_->rol() == "student" && !cerereInCurs;
    const bool cursValid = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt() > 0;
    ui_->refreshLessonCoursesButton->setEnabled(disponibil);
    ui_->lessonCourseCombo->setEnabled(disponibil && ui_->lessonCourseCombo->count() > 0);
    ui_->refreshLessonsButton->setEnabled(disponibil && cursValid);
    ui_->lessonsList->setEnabled(disponibil && cursValid && ui_->lessonsList->count() > 0);
}

void StudentDashboard::incarcaCursurilePentruLectii() {
    if (!poateExecutaCereri(ui_->lessonCoursesStatusLabel)) {
        actualizeazaControaleleLectiilor();
        return;
    }

    const int cursSelectat = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    actualizeazaControaleleLectiilor(true);
    ui_->lessonCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare cursuri..."));
    try {
        const auto cursuri = context_->client().listeazaCursuriInscrise();
        {
            const QSignalBlocker blocare(ui_->lessonCourseCombo);
            ui_->lessonCourseCombo->clear();
            int indexDeSelectat = -1;
            for (const auto& curs : cursuri) {
                ui_->lessonCourseCombo->addItem(QString::fromStdString(curs.nume), curs.id);
                if (curs.id == cursSelectat) indexDeSelectat = ui_->lessonCourseCombo->count() - 1;
            }
            if (!cursuri.empty()) {
                ui_->lessonCourseCombo->setCurrentIndex(indexDeSelectat >= 0 ? indexDeSelectat : 0);
            }
        }
        if (cursuri.empty()) {
            ui_->lessonCoursesStatusLabel->setText(
                QString::fromUtf8(u8"Nu ești înscris la niciun curs."));
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Selectează un curs pentru a vedea lecțiile."));
            ui_->lessonsList->clear();
            lectiiCurente_.clear();
            golesteDetaliileLectiei();
            actualizeazaControaleleLectiilor();
            return;
        }
        ui_->lessonCoursesStatusLabel->clear();
        incarcaLectiileCursului();
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        actualizeazaControaleleLectiilor();
    } catch (const std::exception&) {
        ui_->lessonCoursesStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        actualizeazaControaleleLectiilor();
    }
}

void StudentDashboard::incarcaLectiileCursului() {
    if (!poateExecutaCereri(ui_->lessonsStatusLabel)) {
        actualizeazaControaleleLectiilor();
        return;
    }
    const int cursId = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    if (cursId <= 0) {
        ui_->lessonsStatusLabel->setText(
            QString::fromUtf8(u8"Selectează un curs pentru a vedea lecțiile."));
        ui_->lessonsList->clear();
        lectiiCurente_.clear();
        golesteDetaliileLectiei();
        actualizeazaControaleleLectiilor();
        return;
    }

    const auto* elementSelectat = ui_->lessonsList->currentItem();
    const int lectieSelectata = elementSelectat ? elementSelectat->data(Qt::UserRole).toInt() : 0;
    actualizeazaControaleleLectiilor(true);
    ui_->lessonsStatusLabel->setText(QString::fromUtf8(u8"Încărcare lecții..."));
    try {
        auto lectii = context_->client().listeazaLectii(cursId);
        {
            const QSignalBlocker blocare(ui_->lessonsList);
            ui_->lessonsList->clear();
            lectiiCurente_ = std::move(lectii);
            int randDeSelectat = -1;
            for (const auto& lectie : lectiiCurente_) {
                const QString tip = lectie.tip == TipLectieEdu::Text
                    ? QString::fromUtf8(u8"Text") : QString::fromUtf8(u8"Video");
                auto* element = new QListWidgetItem(
                    QString::fromUtf8("%1 — %2").arg(QString::fromStdString(lectie.nume), tip),
                    ui_->lessonsList);
                element->setData(Qt::UserRole, lectie.id);
                if (lectie.id == lectieSelectata) randDeSelectat = ui_->lessonsList->count() - 1;
            }
            if (!lectiiCurente_.empty()) {
                ui_->lessonsList->setCurrentRow(randDeSelectat >= 0 ? randDeSelectat : 0);
            }
        }
        if (lectiiCurente_.empty()) {
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Cursul selectat nu are lecții."));
            golesteDetaliileLectiei();
        } else {
            ui_->lessonsStatusLabel->clear();
            afiseazaLectiaSelectata();
        }
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->lessonsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    actualizeazaControaleleLectiilor();
}

void StudentDashboard::afiseazaLectiaSelectata() {
    const auto* element = ui_->lessonsList->currentItem();
    const int lectieId = element ? element->data(Qt::UserRole).toInt() : 0;
    const auto lectie = std::find_if(
        lectiiCurente_.cbegin(), lectiiCurente_.cend(),
        [lectieId](const LectiePublicEdu& valoare) { return valoare.id == lectieId; });
    if (lectie == lectiiCurente_.cend()) {
        golesteDetaliileLectiei();
        return;
    }

    ui_->lessonTitleLabel->setText(QString::fromStdString(lectie->nume));
    QStringList metadate;
    metadate << (lectie->tip == TipLectieEdu::Text
                     ? QString::fromUtf8(u8"Tip: Text")
                     : QString::fromUtf8(u8"Tip: Video"));
    metadate << QString::fromUtf8(u8"Dimensiune: %1 octeți").arg(lectie->dimensiuneOcteti);
    if (lectie->numarCuvinte) {
        metadate << QString::fromUtf8(u8"Cuvinte: %1").arg(*lectie->numarCuvinte);
    }
    if (lectie->durata) {
        metadate << QString::fromUtf8(u8"Durată: %1 secunde").arg(*lectie->durata);
    }
    if (lectie->codec) {
        metadate << QString::fromUtf8(u8"Codec: %1")
                         .arg(QString::fromStdString(*lectie->codec));
    }
    ui_->lessonMetadataLabel->setText(metadate.join(QString::fromUtf8(" • ")));
    ui_->lessonContent->setPlainText(QString::fromStdString(lectie->continut));
}
