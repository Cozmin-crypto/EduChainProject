#include "ProfessorDashboard.h"

#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ProtocolEdu.h"
#include "ui_ProfessorDashboard.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QTextEdit>
#include <QVariant>

#include <exception>
#include <utility>

namespace {
bool citesteNumarNenegativ(const QLineEdit* camp, long long& rezultat) {
    bool valid = false;
    const QString text = camp->text().trimmed();
    rezultat = text.toLongLong(&valid);
    return valid && rezultat >= 0;
}

QString numeTip(TipLectieEdu tip) {
    return tip == TipLectieEdu::Text ? QString::fromUtf8(u8"Text")
                                     : QString::fromUtf8(u8"Video");
}
}

ProfessorDashboard::ProfessorDashboard(std::shared_ptr<ApplicationContext> context,
                                       QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::ProfessorDashboard>()),
      context_(std::move(context)) {
    ui_->setupUi(this);
    ui_->userLabel->setText(QString::fromUtf8("Utilizator: %1")
                                .arg(QString::fromStdString(context_->email())));
    ui_->roleLabel->setText(QString::fromUtf8("Rol: %1")
                                .arg(QString::fromStdString(context_->rol())));
    actualizeazaStareConexiune();

    ui_->lessonTypeCombo->addItem(QString::fromUtf8(u8"Text"),
                                  static_cast<int>(TipLectieEdu::Text));
    ui_->lessonTypeCombo->addItem(QString::fromUtf8(u8"Video"),
                                  static_cast<int>(TipLectieEdu::Video));

    connect(ui_->homeButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(0); });
    connect(ui_->myCoursesButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(1); });
    connect(ui_->createCourseButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(2); });
    connect(ui_->lessonsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] {
                ui_->pagesStack->setCurrentIndex(3);
                incarcaCursurilePentruLectii();
            });
    connect(ui_->evaluationsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(4); });
    connect(ui_->studentsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(5); });

    connect(ui_->refreshLessonCoursesButton, &QPushButton::clicked,
            this, &ProfessorDashboard::incarcaCursurilePentruLectii);
    connect(ui_->refreshLessonsButton, &QPushButton::clicked,
            this, &ProfessorDashboard::incarcaLectiileCursului);
    connect(ui_->lessonCourseCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                golesteDetaliileLectiei();
                incarcaLectiileCursului();
            });
    connect(ui_->lessonsList, &QListWidget::currentItemChanged,
            this, &ProfessorDashboard::afiseazaLectiaSelectata);
    connect(ui_->lessonTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { actualizeazaCampurileTipului(); });
    connect(ui_->addLessonButton, &QPushButton::clicked,
            this, &ProfessorDashboard::adaugaLectie);

    golesteDetaliileLectiei();
    golesteFormularul();
    actualizeazaCampurileTipului();
    actualizeazaControalele();
}

ProfessorDashboard::~ProfessorDashboard() = default;

bool ProfessorDashboard::poateExecutaCereri(QLabel* statusLabel) {
    if (!context_->esteAutentificat()) {
        statusLabel->setText(QString::fromUtf8(u8"Sesiunea nu este autentificată."));
        actualizeazaControalele();
        return false;
    }
    if (context_->rol() != "profesor") {
        statusLabel->setText(
            QString::fromUtf8(u8"Operația este permisă numai Profesorului."));
        actualizeazaControalele();
        return false;
    }
    if (!context_->esteConectat()) {
        statusLabel->setText(QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        actualizeazaControalele();
        return false;
    }
    return true;
}

void ProfessorDashboard::actualizeazaStareConexiune() {
    ui_->connectionLabel->setText(
        context_->esteConectat()
            ? QString::fromUtf8("Conectat la %1:%2")
                  .arg(QString::fromStdString(context_->host())).arg(context_->port())
            : QString::fromUtf8(u8"Conexiune pierdută"));
}

void ProfessorDashboard::actualizeazaControalele(bool cerereInCurs) {
    const bool disponibil = context_->esteConectat() && context_->esteAutentificat() &&
                            context_->rol() == "profesor" && !cerereInCurs;
    const bool cursValid = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt() > 0;
    ui_->refreshLessonCoursesButton->setEnabled(disponibil);
    ui_->lessonCourseCombo->setEnabled(disponibil && ui_->lessonCourseCombo->count() > 0);
    ui_->refreshLessonsButton->setEnabled(disponibil && cursValid);
    ui_->lessonsList->setEnabled(disponibil && cursValid && ui_->lessonsList->count() > 0);
    ui_->lessonFormGroup->setEnabled(disponibil && cursValid);
    ui_->addLessonButton->setEnabled(disponibil && cursValid);
}

void ProfessorDashboard::golesteDetaliileLectiei() {
    ui_->lessonDetailTitleLabel->setText(
        QString::fromUtf8(u8"Nicio lecție selectată"));
    ui_->lessonDetailMetadataLabel->clear();
    ui_->lessonDetailContent->clear();
}

void ProfessorDashboard::golesteFormularul() {
    ui_->lessonNameEdit->clear();
    ui_->lessonContentEdit->clear();
    ui_->lessonSizeEdit->clear();
    ui_->lessonWordCountEdit->clear();
    ui_->lessonDurationEdit->clear();
    ui_->lessonCodecEdit->clear();
    ui_->lessonTypeCombo->setCurrentIndex(0);
}

void ProfessorDashboard::actualizeazaCampurileTipului() {
    const auto tip = static_cast<TipLectieEdu>(
        ui_->lessonTypeCombo->currentData(Qt::UserRole).toInt());
    const bool esteText = tip == TipLectieEdu::Text;
    ui_->lessonWordCountLabel->setVisible(esteText);
    ui_->lessonWordCountEdit->setVisible(esteText);
    ui_->lessonDurationLabel->setVisible(!esteText);
    ui_->lessonDurationEdit->setVisible(!esteText);
    ui_->lessonCodecLabel->setVisible(!esteText);
    ui_->lessonCodecEdit->setVisible(!esteText);
}

void ProfessorDashboard::incarcaCursurilePentruLectii() {
    if (!poateExecutaCereri(ui_->lessonCoursesStatusLabel)) return;
    const int cursSelectat = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    const auto profesorId = context_->utilizatorId();
    if (!profesorId || *profesorId <= 0) {
        ui_->lessonCoursesStatusLabel->setText(
            QString::fromUtf8(u8"Sesiunea Profesorului conține un ID invalid."));
        actualizeazaControalele();
        return;
    }

    actualizeazaControalele(true);
    ui_->lessonCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare cursuri..."));
    try {
        const auto cursuri = context_->client().listeazaCursuri();
        QSignalBlocker blocare(ui_->lessonCourseCombo);
        ui_->lessonCourseCombo->clear();
        int indexSelectat = -1;
        for (const auto& curs : cursuri) {
            if (curs.proprietarId != *profesorId) continue;
            ui_->lessonCourseCombo->addItem(QString::fromStdString(curs.nume), curs.id);
            if (curs.id == cursSelectat) indexSelectat = ui_->lessonCourseCombo->count() - 1;
        }
        if (ui_->lessonCourseCombo->count() > 0) {
            ui_->lessonCourseCombo->setCurrentIndex(indexSelectat >= 0 ? indexSelectat : 0);
        }
        blocare.unblock();

        if (ui_->lessonCourseCombo->count() == 0) {
            ui_->lessonCoursesStatusLabel->setText(
                QString::fromUtf8(u8"Nu deții niciun curs."));
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Selectează un curs pentru a vedea lecțiile."));
            ui_->lessonsList->clear();
            golesteDetaliileLectiei();
            actualizeazaControalele();
            return;
        }
        ui_->lessonCoursesStatusLabel->clear();
        incarcaLectiileCursului();
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        golesteDetaliileLectiei();
        actualizeazaStareConexiune();
        actualizeazaControalele();
    } catch (const std::exception&) {
        ui_->lessonCoursesStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        golesteDetaliileLectiei();
        actualizeazaControalele();
    }
}

void ProfessorDashboard::incarcaLectiileCursului() {
    incarcaLectiileCursuluiCuSelectie(0);
}

void ProfessorDashboard::incarcaLectiileCursuluiCuSelectie(int lectiePreferata) {
    if (!poateExecutaCereri(ui_->lessonsStatusLabel)) return;
    const int cursId = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    if (cursId <= 0) {
        ui_->lessonsStatusLabel->setText(
            QString::fromUtf8(u8"Selectează un curs valid."));
        ui_->lessonsList->clear();
        golesteDetaliileLectiei();
        actualizeazaControalele();
        return;
    }
    if (lectiePreferata <= 0 && ui_->lessonsList->currentItem()) {
        lectiePreferata = ui_->lessonsList->currentItem()->data(Qt::UserRole).toInt();
    }

    actualizeazaControalele(true);
    ui_->lessonsStatusLabel->setText(QString::fromUtf8(u8"Încărcare lecții..."));
    golesteDetaliileLectiei();
    try {
        const auto lectii = context_->client().listeazaLectii(cursId);
        QSignalBlocker blocare(ui_->lessonsList);
        ui_->lessonsList->clear();
        int randSelectat = -1;
        for (const auto& lectie : lectii) {
            auto* element = new QListWidgetItem(
                QString::fromUtf8("%1 — %2")
                    .arg(QString::fromStdString(lectie.nume), numeTip(lectie.tip)),
                ui_->lessonsList);
            element->setData(Qt::UserRole, lectie.id);
            if (lectie.id == lectiePreferata) randSelectat = ui_->lessonsList->count() - 1;
        }
        blocare.unblock();

        if (lectii.empty()) {
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Cursul selectat nu are lecții."));
        } else {
            actualizeazaControalele();
            ui_->lessonsList->setCurrentRow(randSelectat >= 0 ? randSelectat : 0);
            ui_->lessonsStatusLabel->clear();
            afiseazaLectiaSelectata();
        }
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        golesteDetaliileLectiei();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->lessonsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        golesteDetaliileLectiei();
    }
    actualizeazaControalele();
}

void ProfessorDashboard::afiseazaLectiaSelectata() {
    golesteDetaliileLectiei();
    if (!poateExecutaCereri(ui_->lessonsStatusLabel)) return;
    const auto* element = ui_->lessonsList->currentItem();
    const int lectieId = element ? element->data(Qt::UserRole).toInt() : 0;
    const int cursId = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    if (lectieId <= 0 || cursId <= 0) {
        if (element) ui_->lessonsStatusLabel->setText(QString::fromUtf8(u8"Selecția lecției este invalidă."));
        return;
    }
    try {
        const auto lectie = context_->client().obtineLectie(lectieId);
        if (!lectie) {
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Lecția nu mai există. Reîmprospătează lista."));
            return;
        }
        if (lectie->id != lectieId || lectie->cursId != cursId) {
            ui_->lessonsStatusLabel->setText(
                QString::fromUtf8(u8"Serverul a returnat date necorespunzătoare selecției."));
            return;
        }
        ui_->lessonDetailTitleLabel->setText(QString::fromStdString(lectie->nume));
        QStringList metadate;
        metadate << QString::fromUtf8(u8"Tip: %1").arg(numeTip(lectie->tip));
        metadate << QString::fromUtf8(u8"Dimensiune: %1 octeți").arg(lectie->dimensiuneOcteti);
        if (lectie->numarCuvinte) metadate << QString::fromUtf8(u8"Cuvinte: %1").arg(*lectie->numarCuvinte);
        if (lectie->durata) metadate << QString::fromUtf8(u8"Durată: %1 secunde").arg(*lectie->durata);
        if (lectie->codec) metadate << QString::fromUtf8(u8"Codec: %1").arg(QString::fromStdString(*lectie->codec));
        ui_->lessonDetailMetadataLabel->setText(metadate.join(QString::fromUtf8(" • ")));
        ui_->lessonDetailContent->setPlainText(QString::fromStdString(lectie->continut));
        ui_->lessonsStatusLabel->clear();
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
        actualizeazaControalele();
    } catch (const std::exception&) {
        ui_->lessonsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
}

void ProfessorDashboard::adaugaLectie() {
    ui_->lessonFormStatusLabel->clear();
    if (!poateExecutaCereri(ui_->lessonFormStatusLabel)) return;
    const int cursId = ui_->lessonCourseCombo->currentData(Qt::UserRole).toInt();
    if (cursId <= 0) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Selectează un curs valid."));
        return;
    }
    const QString nume = ui_->lessonNameEdit->text().trimmed();
    const QString continut = ui_->lessonContentEdit->toPlainText();
    if (nume.isEmpty()) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Numele lecției nu poate fi gol."));
        return;
    }
    if (continut.trimmed().isEmpty()) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Conținutul lecției nu poate fi gol."));
        return;
    }
    long long dimensiune = 0;
    if (!citesteNumarNenegativ(ui_->lessonSizeEdit, dimensiune)) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Dimensiunea trebuie să fie un număr întreg nenegativ."));
        return;
    }
    const auto tip = static_cast<TipLectieEdu>(ui_->lessonTypeCombo->currentData(Qt::UserRole).toInt());
    if (tip != TipLectieEdu::Text && tip != TipLectieEdu::Video) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Selectează un tip valid de lecție."));
        return;
    }

    long long valoareSpecifică = 0;
    QString codec;
    if (tip == TipLectieEdu::Text) {
        if (!citesteNumarNenegativ(ui_->lessonWordCountEdit, valoareSpecifică)) {
            ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Numărul de cuvinte trebuie să fie un întreg nenegativ."));
            return;
        }
    } else {
        if (!citesteNumarNenegativ(ui_->lessonDurationEdit, valoareSpecifică)) {
            ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Durata trebuie să fie un întreg nenegativ."));
            return;
        }
        codec = ui_->lessonCodecEdit->text().trimmed();
        if (codec.isEmpty()) {
            ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Codecul lecției video nu poate fi gol."));
            return;
        }
    }

    actualizeazaControalele(true);
    ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Creare lecție în curs..."));
    try {
        const int lectieId = tip == TipLectieEdu::Text
            ? context_->client().creeazaLectieText(
                  cursId, nume.toUtf8().toStdString(), continut.toUtf8().toStdString(),
                  dimensiune, valoareSpecifică)
            : context_->client().creeazaLectieVideo(
                  cursId, nume.toUtf8().toStdString(), continut.toUtf8().toStdString(),
                  dimensiune, valoareSpecifică, codec.toUtf8().toStdString());
        if (lectieId <= 0) throw ExceptieEdu("Serverul a returnat un ID invalid pentru lectie.");
        golesteFormularul();
        incarcaLectiileCursuluiCuSelectie(lectieId);
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"Lecția a fost creată cu succes."));
    } catch (const ExceptieEdu& eroare) {
        ui_->lessonFormStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        golesteDetaliileLectiei();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->lessonFormStatusLabel->setText(QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        golesteDetaliileLectiei();
    }
    actualizeazaControalele();
}
