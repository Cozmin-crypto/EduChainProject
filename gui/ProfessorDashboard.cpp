#include "ProfessorDashboard.h"

#include "ApplicationContext.h"
#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ProtocolEdu.h"
#include "ui_ProfessorDashboard.h"

#include <QComboBox>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QTextEdit>
#include <QTableWidget>
#include <QTableWidgetItem>
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

QString numeTipEvaluare(TipEvaluareEdu tip) {
    return tip == TipEvaluareEdu::Chestionar
        ? QString::fromUtf8(u8"Chestionar")
        : QString::fromUtf8(u8"Examen final");
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
    ui_->evaluationTypeCombo->addItem(QString::fromUtf8(u8"Chestionar"),
                                      static_cast<int>(TipEvaluareEdu::Chestionar));
    ui_->evaluationTypeCombo->addItem(QString::fromUtf8(u8"Examen final"),
                                      static_cast<int>(TipEvaluareEdu::ExamenFinal));

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
            [this] {
                ui_->pagesStack->setCurrentIndex(4);
                incarcaCursurilePentruEvaluari();
            });
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
    connect(ui_->refreshEvaluationCoursesButton, &QPushButton::clicked,
            this, &ProfessorDashboard::incarcaCursurilePentruEvaluari);
    connect(ui_->refreshEvaluationsButton, &QPushButton::clicked,
            this, &ProfessorDashboard::incarcaEvaluarileCursului);
    connect(ui_->evaluationCourseCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) {
                golesteDetaliileEvaluarii();
                incarcaEvaluarileCursului();
            });
    connect(ui_->evaluationsList, &QListWidget::currentItemChanged,
            this, &ProfessorDashboard::afiseazaEvaluareaSelectata);
    connect(ui_->evaluationTypeCombo, qOverload<int>(&QComboBox::currentIndexChanged),
            this, [this](int) { actualizeazaCampurileTipuluiEvaluarii(); });
    connect(ui_->createEvaluationButton, &QPushButton::clicked,
            this, &ProfessorDashboard::creeazaEvaluare);
    connect(ui_->addQuestionButton, &QPushButton::clicked,
            this, &ProfessorDashboard::adaugaIntrebare);

    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);

    golesteDetaliileLectiei();
    golesteFormularul();
    actualizeazaCampurileTipului();
    actualizeazaControalele();
    golesteDetaliileEvaluarii();
    golesteFormularulEvaluarii();
    golesteFormularulIntrebarii();
    actualizeazaCampurileTipuluiEvaluarii();
    actualizeazaControaleleEvaluarilor();
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

void ProfessorDashboard::actualizeazaControaleleEvaluarilor(bool cerereInCurs) {
    const bool disponibil = context_->esteConectat() && context_->esteAutentificat() &&
                            context_->rol() == "profesor" && !cerereInCurs;
    const bool cursValid = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt() > 0;
    const auto* element = ui_->evaluationsList->currentItem();
    const bool evaluareValida = element && element->data(Qt::UserRole).toInt() > 0;
    const bool chestionar = evaluareValida &&
        element->data(Qt::UserRole + 1).toInt() == static_cast<int>(TipEvaluareEdu::Chestionar);

    ui_->refreshEvaluationCoursesButton->setEnabled(disponibil);
    ui_->evaluationCourseCombo->setEnabled(
        disponibil && ui_->evaluationCourseCombo->count() > 0);
    ui_->refreshEvaluationsButton->setEnabled(disponibil && cursValid);
    ui_->evaluationsList->setEnabled(
        disponibil && cursValid && ui_->evaluationsList->count() > 0);
    ui_->evaluationFormGroup->setEnabled(disponibil && cursValid);
    ui_->createEvaluationButton->setEnabled(disponibil && cursValid);
    ui_->questionFormGroup->setEnabled(disponibil && chestionar);
    ui_->addQuestionButton->setEnabled(disponibil && chestionar);
}

void ProfessorDashboard::golesteDetaliileEvaluarii() {
    ui_->evaluationDetailTitleLabel->setText(
        QString::fromUtf8(u8"Nicio evaluare selectată"));
    ui_->evaluationDetailMetadataLabel->clear();
    ui_->questionsTable->setRowCount(0);
    ui_->questionsStatusLabel->clear();
}

void ProfessorDashboard::golesteFormularulEvaluarii() {
    ui_->evaluationNameEdit->clear();
    ui_->evaluationTypeCombo->setCurrentIndex(0);
    ui_->evaluationTimeSpin->setValue(0);
    ui_->evaluationRequiredCheck->setChecked(false);
    ui_->evaluationQuestionCountSpin->setValue(0);
    ui_->evaluationWeightSpin->setValue(0.0);
}

void ProfessorDashboard::golesteFormularulIntrebarii() {
    ui_->questionTextEdit->clear();
    ui_->correctAnswerEdit->clear();
    ui_->questionPointsSpin->setValue(1.0);
    ui_->questionOrderSpin->setValue(ui_->questionsTable->rowCount());
}

void ProfessorDashboard::actualizeazaCampurileTipuluiEvaluarii() {
    const auto tip = static_cast<TipEvaluareEdu>(
        ui_->evaluationTypeCombo->currentData(Qt::UserRole).toInt());
    const bool chestionar = tip == TipEvaluareEdu::Chestionar;
    ui_->evaluationQuestionCountLabel->setVisible(chestionar);
    ui_->evaluationQuestionCountSpin->setVisible(chestionar);
    ui_->evaluationWeightLabel->setVisible(!chestionar);
    ui_->evaluationWeightSpin->setVisible(!chestionar);
}

void ProfessorDashboard::incarcaCursurilePentruEvaluari() {
    if (!poateExecutaCereri(ui_->evaluationCoursesStatusLabel)) {
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const auto profesorId = context_->utilizatorId();
    if (!profesorId || *profesorId <= 0) {
        ui_->evaluationCoursesStatusLabel->setText(
            QString::fromUtf8(u8"Sesiunea Profesorului conține un ID invalid."));
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const int cursSelectat = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    actualizeazaControaleleEvaluarilor(true);
    ui_->evaluationCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare cursuri..."));
    try {
        const auto cursuri = context_->client().listeazaCursuri();
        QSignalBlocker blocare(ui_->evaluationCourseCombo);
        ui_->evaluationCourseCombo->clear();
        int indexSelectat = -1;
        for (const auto& curs : cursuri) {
            if (curs.proprietarId != *profesorId) continue;
            ui_->evaluationCourseCombo->addItem(QString::fromStdString(curs.nume), curs.id);
            if (curs.id == cursSelectat) indexSelectat = ui_->evaluationCourseCombo->count() - 1;
        }
        if (ui_->evaluationCourseCombo->count() > 0) {
            ui_->evaluationCourseCombo->setCurrentIndex(indexSelectat >= 0 ? indexSelectat : 0);
        }
        blocare.unblock();
        if (ui_->evaluationCourseCombo->count() == 0) {
            ui_->evaluationCoursesStatusLabel->setText(
                QString::fromUtf8(u8"Nu deții niciun curs."));
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Selectează un curs pentru a vedea evaluările."));
            ui_->evaluationsList->clear();
            golesteDetaliileEvaluarii();
            actualizeazaControaleleEvaluarilor();
            return;
        }
        ui_->evaluationCoursesStatusLabel->clear();
        incarcaEvaluarileCursului();
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        ui_->evaluationCourseCombo->clear();
        ui_->evaluationsList->clear();
        golesteDetaliileEvaluarii();
        actualizeazaStareConexiune();
        actualizeazaControaleleEvaluarilor();
    } catch (const std::exception&) {
        ui_->evaluationCoursesStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        ui_->evaluationCourseCombo->clear();
        ui_->evaluationsList->clear();
        golesteDetaliileEvaluarii();
        actualizeazaControaleleEvaluarilor();
    }
}

void ProfessorDashboard::incarcaEvaluarileCursului() {
    incarcaEvaluarileCursuluiCuSelectie(0);
}

void ProfessorDashboard::incarcaEvaluarileCursuluiCuSelectie(int evaluarePreferata) {
    if (!poateExecutaCereri(ui_->evaluationsStatusLabel)) {
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const int cursId = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    if (cursId <= 0) {
        ui_->evaluationsStatusLabel->setText(QString::fromUtf8(u8"Selectează un curs valid."));
        ui_->evaluationsList->clear();
        golesteDetaliileEvaluarii();
        actualizeazaControaleleEvaluarilor();
        return;
    }
    if (evaluarePreferata <= 0 && ui_->evaluationsList->currentItem()) {
        evaluarePreferata = ui_->evaluationsList->currentItem()->data(Qt::UserRole).toInt();
    }
    actualizeazaControaleleEvaluarilor(true);
    ui_->evaluationsStatusLabel->setText(QString::fromUtf8(u8"Încărcare evaluări..."));
    golesteDetaliileEvaluarii();
    try {
        const auto evaluari = context_->client().listeazaEvaluari(cursId);
        QSignalBlocker blocare(ui_->evaluationsList);
        ui_->evaluationsList->clear();
        int randSelectat = -1;
        for (const auto& evaluare : evaluari) {
            auto* element = new QListWidgetItem(
                QString::fromUtf8(u8"%1 — %2")
                    .arg(QString::fromStdString(evaluare.nume), numeTipEvaluare(evaluare.tip)),
                ui_->evaluationsList);
            element->setData(Qt::UserRole, evaluare.id);
            element->setData(Qt::UserRole + 1, static_cast<int>(evaluare.tip));
            if (evaluare.id == evaluarePreferata) randSelectat = ui_->evaluationsList->count() - 1;
        }
        blocare.unblock();
        if (evaluari.empty()) {
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Cursul selectat nu are evaluări."));
        } else {
            ui_->evaluationsList->setCurrentRow(randSelectat >= 0 ? randSelectat : 0);
            ui_->evaluationsStatusLabel->clear();
            afiseazaEvaluareaSelectata();
        }
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        ui_->evaluationsList->clear();
        golesteDetaliileEvaluarii();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        ui_->evaluationsList->clear();
        golesteDetaliileEvaluarii();
    }
    actualizeazaControaleleEvaluarilor();
}

void ProfessorDashboard::afiseazaEvaluareaSelectata() {
    golesteDetaliileEvaluarii();
    if (!poateExecutaCereri(ui_->evaluationsStatusLabel)) {
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const auto* element = ui_->evaluationsList->currentItem();
    const int evaluareId = element ? element->data(Qt::UserRole).toInt() : 0;
    const int cursId = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    if (evaluareId <= 0 || cursId <= 0) {
        if (element) ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"Selecția evaluării este invalidă."));
        actualizeazaControaleleEvaluarilor();
        return;
    }
    actualizeazaControaleleEvaluarilor(true);
    try {
        const auto evaluare = context_->client().obtineEvaluare(evaluareId);
        if (!evaluare) {
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Evaluarea nu mai există. Reîmprospătează lista."));
            actualizeazaControaleleEvaluarilor();
            return;
        }
        if (evaluare->id != evaluareId || evaluare->cursId != cursId) {
            throw ExceptieEdu("Serverul a returnat o evaluare necorespunzatoare selectiei.");
        }
        ui_->evaluationDetailTitleLabel->setText(QString::fromStdString(evaluare->nume));
        QStringList metadate;
        metadate << QString::fromUtf8(u8"Tip: %1").arg(numeTipEvaluare(evaluare->tip));
        metadate << QString::fromUtf8(u8"Limită: %1 minute").arg(evaluare->limitaTimp);
        metadate << (evaluare->esteObligatorie ? QString::fromUtf8(u8"Obligatorie")
                                               : QString::fromUtf8(u8"Opțională"));
        if (evaluare->numarIntrebari) {
            metadate << QString::fromUtf8(u8"Număr întrebări: %1").arg(*evaluare->numarIntrebari);
        }
        if (evaluare->pondere) {
            metadate << QString::fromUtf8(u8"Pondere: %1").arg(*evaluare->pondere);
        }
        ui_->evaluationDetailMetadataLabel->setText(
            metadate.join(QString::fromUtf8(u8" • ")));

        const auto intrebari = context_->client().listeazaIntrebari(evaluareId);
        ui_->questionsTable->setRowCount(static_cast<int>(intrebari.size()));
        int rand = 0;
        for (const auto& intrebare : intrebari) {
            auto* ordine = new QTableWidgetItem(QString::number(intrebare.ordine));
            ordine->setData(Qt::UserRole, intrebare.id);
            ui_->questionsTable->setItem(rand, 0, ordine);
            ui_->questionsTable->setItem(rand, 1,
                new QTableWidgetItem(QString::fromStdString(intrebare.enunt)));
            ui_->questionsTable->setItem(rand, 2,
                new QTableWidgetItem(QString::number(intrebare.punctajMaxim)));
            ui_->questionsTable->setItem(rand, 3,
                new QTableWidgetItem(QString::number(intrebare.id)));
            ++rand;
        }
        ui_->questionsStatusLabel->setText(
            intrebari.empty() ? QString::fromUtf8(u8"Evaluarea nu are întrebări.") : QString());
        ui_->questionOrderSpin->setValue(static_cast<int>(intrebari.size()));
        ui_->evaluationsStatusLabel->clear();
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        golesteDetaliileEvaluarii();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        golesteDetaliileEvaluarii();
    }
    actualizeazaControaleleEvaluarilor();
}

void ProfessorDashboard::creeazaEvaluare() {
    ui_->evaluationFormStatusLabel->clear();
    if (!poateExecutaCereri(ui_->evaluationFormStatusLabel)) {
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const int cursId = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    const QString nume = ui_->evaluationNameEdit->text().trimmed();
    if (cursId <= 0) {
        ui_->evaluationFormStatusLabel->setText(QString::fromUtf8(u8"Selectează un curs valid."));
        return;
    }
    if (nume.isEmpty()) {
        ui_->evaluationFormStatusLabel->setText(
            QString::fromUtf8(u8"Titlul evaluării nu poate fi gol."));
        return;
    }
    const auto tip = static_cast<TipEvaluareEdu>(
        ui_->evaluationTypeCombo->currentData(Qt::UserRole).toInt());
    if (tip != TipEvaluareEdu::Chestionar && tip != TipEvaluareEdu::ExamenFinal) {
        ui_->evaluationFormStatusLabel->setText(
            QString::fromUtf8(u8"Selectează un tip valid de evaluare."));
        return;
    }
    CerereSalvareEvaluareClient cerere;
    cerere.cursId = cursId;
    cerere.nume = nume.toUtf8().toStdString();
    cerere.limitaTimp = ui_->evaluationTimeSpin->value();
    cerere.esteObligatorie = ui_->evaluationRequiredCheck->isChecked();
    if (tip == TipEvaluareEdu::Chestionar) {
        cerere.numarIntrebari = ui_->evaluationQuestionCountSpin->value();
    } else {
        cerere.pondere = ui_->evaluationWeightSpin->value();
    }

    actualizeazaControaleleEvaluarilor(true);
    ui_->evaluationFormStatusLabel->setText(QString::fromUtf8(u8"Creare evaluare..."));
    try {
        const int evaluareId = tip == TipEvaluareEdu::Chestionar
            ? context_->client().creeazaChestionar(cerere)
            : context_->client().creeazaExamenFinal(cerere);
        if (evaluareId <= 0) throw ExceptieEdu("Serverul a returnat un ID invalid pentru evaluare.");
        golesteFormularulEvaluarii();
        incarcaEvaluarileCursuluiCuSelectie(evaluareId);
        ui_->evaluationFormStatusLabel->setText(
            QString::fromUtf8(u8"Evaluarea a fost creată cu succes."));
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationFormStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationFormStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    actualizeazaControaleleEvaluarilor();
}

void ProfessorDashboard::adaugaIntrebare() {
    ui_->questionFormStatusLabel->clear();
    if (!poateExecutaCereri(ui_->questionFormStatusLabel)) {
        actualizeazaControaleleEvaluarilor();
        return;
    }
    const auto* element = ui_->evaluationsList->currentItem();
    const int evaluareId = element ? element->data(Qt::UserRole).toInt() : 0;
    if (evaluareId <= 0) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Selectează o evaluare validă."));
        return;
    }
    if (element->data(Qt::UserRole + 1).toInt() !=
        static_cast<int>(TipEvaluareEdu::Chestionar)) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Întrebările pot fi adăugate numai unui chestionar."));
        return;
    }
    const QString enunt = ui_->questionTextEdit->text().trimmed();
    const QString raspuns = ui_->correctAnswerEdit->text().trimmed();
    if (enunt.isEmpty()) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Textul întrebării nu poate fi gol."));
        return;
    }
    if (raspuns.isEmpty()) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Răspunsul corect nu poate fi gol."));
        return;
    }
    const double punctaj = ui_->questionPointsSpin->value();
    if (punctaj <= 0.0) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Punctajul trebuie să fie strict pozitiv."));
        return;
    }
    const long long ordine = ui_->questionOrderSpin->value();
    if (ordine < 0) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Ordinea nu poate fi negativă."));
        return;
    }

    actualizeazaControaleleEvaluarilor(true);
    ui_->questionFormStatusLabel->setText(QString::fromUtf8(u8"Adăugare întrebare..."));
    try {
        const int intrebareId = context_->client().adaugaIntrebare(
            evaluareId, enunt.toUtf8().toStdString(), raspuns.toUtf8().toStdString(),
            punctaj, ordine);
        if (intrebareId <= 0) throw ExceptieEdu("Serverul a returnat un ID invalid pentru intrebare.");
        golesteFormularulIntrebarii();
        afiseazaEvaluareaSelectata();
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"Întrebarea a fost adăugată cu succes."));
    } catch (const ExceptieEdu& eroare) {
        ui_->questionFormStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->questionFormStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    actualizeazaControaleleEvaluarilor();
}
