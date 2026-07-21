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
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QTextBrowser>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <unordered_set>

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
            [this] {
                ui_->pagesStack->setCurrentIndex(4);
                incarcaCursurilePentruEvaluari();
            });
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
    connect(ui_->refreshEvaluationCoursesButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaCursurilePentruEvaluari);
    connect(ui_->refreshEvaluationsButton, &QPushButton::clicked,
            this, &StudentDashboard::incarcaEvaluarileCursului);
    connect(ui_->evaluationCourseCombo,
            qOverload<int>(&QComboBox::currentIndexChanged), this,
            [this](int) {
                golesteEvaluarea();
                incarcaEvaluarileCursului();
            });
    connect(ui_->evaluationsList, &QListWidget::currentItemChanged,
            this, &StudentDashboard::afiseazaEvaluareaSelectata);
    connect(ui_->startEvaluationButton, &QPushButton::clicked,
            this, &StudentDashboard::pornesteEvaluarea);
    connect(ui_->finalizeEvaluationButton, &QPushButton::clicked,
            this, &StudentDashboard::finalizeazaEvaluarea);
    ui_->enrollButton->setEnabled(false);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui_->questionsTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    golesteDetaliileLectiei();
    actualizeazaControaleleLectiilor();
    golesteEvaluarea();
    actualizeazaControaleleEvaluarii();
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

void StudentDashboard::golesteEvaluarea() {
    ui_->evaluationTitleLabel->setText(QString::fromUtf8(u8"Nicio evaluare selectată"));
    ui_->evaluationMetadataLabel->clear();
    ui_->questionsTable->setRowCount(0);
    ui_->evaluationProgressLabel->setText(QString::fromUtf8(u8"Progres: 0/0"));
    ui_->evaluationActionStatusLabel->clear();
    ui_->evaluationResultLabel->clear();
    intrebariEvaluare_.clear();
    incercareEvaluareId_ = 0;
    incercareEvaluareFinalizata_ = false;
    finalizareEvaluareInCurs_ = false;
}

void StudentDashboard::actualizeazaControaleleEvaluarii(bool cerereInCurs) {
    const bool disponibil = context_->esteConectat() && context_->esteAutentificat() &&
                            context_->rol() == "student" && !cerereInCurs;
    const bool cursValid = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt() > 0;
    const auto* evaluare = ui_->evaluationsList->currentItem();
    const bool evaluareValida = evaluare && evaluare->data(Qt::UserRole).toInt() > 0;
    const bool incercareActiva = incercareEvaluareId_ > 0 &&
                                 !incercareEvaluareFinalizata_;

    ui_->refreshEvaluationCoursesButton->setEnabled(disponibil && !incercareActiva);
    ui_->evaluationCourseCombo->setEnabled(
        disponibil && !incercareActiva && ui_->evaluationCourseCombo->count() > 0);
    ui_->refreshEvaluationsButton->setEnabled(disponibil && cursValid && !incercareActiva);
    ui_->evaluationsList->setEnabled(
        disponibil && cursValid && !incercareActiva && ui_->evaluationsList->count() > 0);
    ui_->startEvaluationButton->setEnabled(
        disponibil && evaluareValida && !intrebariEvaluare_.empty() &&
        incercareEvaluareId_ == 0 && !incercareEvaluareFinalizata_);

    for (int rand = 0; rand < ui_->questionsTable->rowCount(); ++rand) {
        auto* raspuns = qobject_cast<QLineEdit*>(ui_->questionsTable->cellWidget(rand, 3));
        if (raspuns) {
            raspuns->setEnabled(disponibil && incercareActiva && !finalizareEvaluareInCurs_);
        }
    }
    ui_->finalizeEvaluationButton->setEnabled(
        disponibil && incercareActiva && !finalizareEvaluareInCurs_);
}

void StudentDashboard::incarcaCursurilePentruEvaluari() {
    if (!poateExecutaCereri(ui_->evaluationCoursesStatusLabel)) {
        actualizeazaControaleleEvaluarii();
        return;
    }
    if (incercareEvaluareId_ > 0 && !incercareEvaluareFinalizata_) {
        ui_->evaluationCoursesStatusLabel->setText(
            QString::fromUtf8(u8"Finalizează încercarea curentă înainte de a schimba cursul."));
        return;
    }

    const int cursSelectat = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    actualizeazaControaleleEvaluarii(true);
    ui_->evaluationCoursesStatusLabel->setText(QString::fromUtf8(u8"Încărcare cursuri..."));
    try {
        const auto cursuri = context_->client().listeazaCursuriInscrise();
        QSignalBlocker blocare(ui_->evaluationCourseCombo);
        ui_->evaluationCourseCombo->clear();
        int indexSelectat = -1;
        for (const auto& curs : cursuri) {
            ui_->evaluationCourseCombo->addItem(QString::fromStdString(curs.nume), curs.id);
            if (curs.id == cursSelectat) indexSelectat = ui_->evaluationCourseCombo->count() - 1;
        }
        if (!cursuri.empty()) {
            ui_->evaluationCourseCombo->setCurrentIndex(indexSelectat >= 0 ? indexSelectat : 0);
        }
        blocare.unblock();

        if (cursuri.empty()) {
            ui_->evaluationCoursesStatusLabel->setText(
                QString::fromUtf8(u8"Nu ești înscris la niciun curs."));
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Selectează un curs pentru a vedea evaluările."));
            ui_->evaluationsList->clear();
            golesteEvaluarea();
            actualizeazaControaleleEvaluarii();
            return;
        }
        ui_->evaluationCoursesStatusLabel->clear();
        incarcaEvaluarileCursului();
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationCoursesStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        ui_->evaluationsList->clear();
        golesteEvaluarea();
        actualizeazaStareConexiune();
        actualizeazaControaleleEvaluarii();
    } catch (const std::exception&) {
        ui_->evaluationCoursesStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        ui_->evaluationsList->clear();
        golesteEvaluarea();
        actualizeazaControaleleEvaluarii();
    }
}

void StudentDashboard::incarcaEvaluarileCursului() {
    incarcaEvaluarileCursuluiCuSelectie(0);
}

void StudentDashboard::incarcaEvaluarileCursuluiCuSelectie(int evaluarePreferata) {
    if (!poateExecutaCereri(ui_->evaluationsStatusLabel)) {
        actualizeazaControaleleEvaluarii();
        return;
    }
    if (incercareEvaluareId_ > 0 && !incercareEvaluareFinalizata_) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"Finalizează încercarea curentă înainte de refresh."));
        return;
    }
    const int cursId = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    if (cursId <= 0) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"Selectează un curs valid."));
        ui_->evaluationsList->clear();
        golesteEvaluarea();
        actualizeazaControaleleEvaluarii();
        return;
    }
    if (evaluarePreferata <= 0 && ui_->evaluationsList->currentItem()) {
        evaluarePreferata = ui_->evaluationsList->currentItem()->data(Qt::UserRole).toInt();
    }

    actualizeazaControaleleEvaluarii(true);
    ui_->evaluationsStatusLabel->setText(QString::fromUtf8(u8"Încărcare evaluări..."));
    try {
        const auto evaluari = context_->client().listeazaEvaluari(cursId);
        QSignalBlocker blocare(ui_->evaluationsList);
        ui_->evaluationsList->clear();
        int randSelectat = -1;
        for (const auto& evaluare : evaluari) {
            const QString tip = evaluare.tip == TipEvaluareEdu::Chestionar
                ? QString::fromUtf8(u8"Chestionar")
                : QString::fromUtf8(u8"Examen final");
            auto* element = new QListWidgetItem(
                QString::fromUtf8("%1 — %2")
                    .arg(QString::fromStdString(evaluare.nume), tip),
                ui_->evaluationsList);
            element->setData(Qt::UserRole, evaluare.id);
            if (evaluare.id == evaluarePreferata) randSelectat = ui_->evaluationsList->count() - 1;
        }
        blocare.unblock();
        golesteEvaluarea();

        if (evaluari.empty()) {
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Cursul selectat nu are evaluări."));
        } else {
            actualizeazaControaleleEvaluarii();
            const QSignalBlocker blocareSelectie(ui_->evaluationsList);
            ui_->evaluationsList->setCurrentRow(randSelectat >= 0 ? randSelectat : 0);
            ui_->evaluationsStatusLabel->clear();
            afiseazaEvaluareaSelectata();
        }
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        ui_->evaluationsList->clear();
        golesteEvaluarea();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        ui_->evaluationsList->clear();
        golesteEvaluarea();
    }
    actualizeazaControaleleEvaluarii();
}

void StudentDashboard::afiseazaEvaluareaSelectata() {
    golesteEvaluarea();
    if (!poateExecutaCereri(ui_->evaluationsStatusLabel)) {
        actualizeazaControaleleEvaluarii();
        return;
    }
    const auto* element = ui_->evaluationsList->currentItem();
    const int evaluareId = element ? element->data(Qt::UserRole).toInt() : 0;
    const int cursId = ui_->evaluationCourseCombo->currentData(Qt::UserRole).toInt();
    if (evaluareId <= 0 || cursId <= 0) {
        if (element) ui_->evaluationsStatusLabel->setText(QString::fromUtf8(u8"Selecția evaluării este invalidă."));
        actualizeazaControaleleEvaluarii();
        return;
    }

    actualizeazaControaleleEvaluarii(true);
    ui_->evaluationsStatusLabel->setText(QString::fromUtf8(u8"Încărcare evaluare..."));
    try {
        const auto evaluare = context_->client().obtineEvaluare(evaluareId);
        if (!evaluare) {
            ui_->evaluationsStatusLabel->setText(
                QString::fromUtf8(u8"Evaluarea nu mai există. Reîmprospătează lista."));
            actualizeazaControaleleEvaluarii();
            return;
        }
        if (evaluare->id != evaluareId || evaluare->cursId != cursId) {
            throw ExceptieEdu("Serverul a returnat o evaluare necorespunzatoare selectiei.");
        }
        auto intrebari = context_->client().listeazaIntrebari(evaluareId);
        std::unordered_set<int> iduri;
        double punctajMaxim = 0.0;
        for (const auto& intrebare : intrebari) {
            if (intrebare.id <= 0 || intrebare.evaluareId != evaluareId ||
                intrebare.enunt.empty() || !std::isfinite(intrebare.punctajMaxim) ||
                intrebare.punctajMaxim < 0.0 || !iduri.insert(intrebare.id).second) {
                throw ExceptieEdu("Serverul a returnat o intrebare invalida.");
            }
            punctajMaxim += intrebare.punctajMaxim;
        }
        if (!std::isfinite(punctajMaxim)) {
            throw ExceptieEdu("Punctajul maxim al evaluarii este invalid.");
        }
        intrebariEvaluare_ = std::move(intrebari);

        ui_->evaluationTitleLabel->setText(QString::fromStdString(evaluare->nume));
        QStringList metadate;
        metadate << (evaluare->tip == TipEvaluareEdu::Chestionar
                         ? QString::fromUtf8(u8"Tip: Chestionar")
                         : QString::fromUtf8(u8"Tip: Examen final"));
        metadate << QString::fromUtf8(u8"Limită: %1 minute").arg(evaluare->limitaTimp);
        metadate << (evaluare->esteObligatorie
                         ? QString::fromUtf8(u8"Obligatorie")
                         : QString::fromUtf8(u8"Opțională"));
        if (evaluare->numarIntrebari) {
            metadate << QString::fromUtf8(u8"Întrebări declarate: %1").arg(*evaluare->numarIntrebari);
        }
        if (evaluare->pondere) {
            metadate << QString::fromUtf8(u8"Pondere: %1").arg(*evaluare->pondere);
        }
        metadate << QString::fromUtf8(u8"Punctaj maxim: %1").arg(punctajMaxim);
        ui_->evaluationMetadataLabel->setText(metadate.join(QString::fromUtf8(" • ")));

        ui_->questionsTable->setRowCount(static_cast<int>(intrebariEvaluare_.size()));
        for (int rand = 0; rand < static_cast<int>(intrebariEvaluare_.size()); ++rand) {
            const auto& intrebare = intrebariEvaluare_[static_cast<std::size_t>(rand)];
            auto* ordine = new QTableWidgetItem(QString::number(intrebare.ordine + 1));
            ordine->setData(Qt::UserRole, intrebare.id);
            ordine->setFlags(ordine->flags() & ~Qt::ItemIsEditable);
            auto* enunt = new QTableWidgetItem(QString::fromStdString(intrebare.enunt));
            enunt->setFlags(enunt->flags() & ~Qt::ItemIsEditable);
            auto* punctaj = new QTableWidgetItem(QString::number(intrebare.punctajMaxim));
            punctaj->setFlags(punctaj->flags() & ~Qt::ItemIsEditable);
            auto* raspuns = new QLineEdit(ui_->questionsTable);
            raspuns->setPlaceholderText(QString::fromUtf8(u8"Răspunsul tău"));
            raspuns->setEnabled(false);
            connect(raspuns, &QLineEdit::textChanged,
                    this, &StudentDashboard::actualizeazaProgresulEvaluarii);
            ui_->questionsTable->setItem(rand, 0, ordine);
            ui_->questionsTable->setItem(rand, 1, enunt);
            ui_->questionsTable->setItem(rand, 2, punctaj);
            ui_->questionsTable->setCellWidget(rand, 3, raspuns);
        }
        actualizeazaProgresulEvaluarii();
        ui_->evaluationsStatusLabel->setText(
            intrebariEvaluare_.empty()
                ? QString::fromUtf8(u8"Evaluarea nu conține întrebări disponibile.")
                : QString());
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationsStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        golesteEvaluarea();
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationsStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
        golesteEvaluarea();
    }
    actualizeazaControaleleEvaluarii();
}

void StudentDashboard::pornesteEvaluarea() {
    if (!poateExecutaCereri(ui_->evaluationActionStatusLabel)) {
        actualizeazaControaleleEvaluarii();
        return;
    }
    const auto* element = ui_->evaluationsList->currentItem();
    const int evaluareId = element ? element->data(Qt::UserRole).toInt() : 0;
    if (evaluareId <= 0 || intrebariEvaluare_.empty()) {
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Evaluarea selectată nu poate fi începută."));
        return;
    }
    if (incercareEvaluareId_ > 0) {
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Există deja o încercare deschisă pentru această selecție."));
        return;
    }

    actualizeazaControaleleEvaluarii(true);
    ui_->evaluationActionStatusLabel->setText(QString::fromUtf8(u8"Pornire evaluare..."));
    try {
        incercareEvaluareId_ = context_->client().pornesteIncercare(evaluareId);
        if (incercareEvaluareId_ <= 0) {
            incercareEvaluareId_ = 0;
            throw ExceptieEdu("Serverul a returnat un ID invalid pentru incercare.");
        }
        incercareEvaluareFinalizata_ = false;
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Evaluarea a început. Completează toate răspunsurile."));
    } catch (const ExceptieEdu& eroare) {
        incercareEvaluareId_ = 0;
        ui_->evaluationActionStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        incercareEvaluareId_ = 0;
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    actualizeazaControaleleEvaluarii();
}

void StudentDashboard::actualizeazaProgresulEvaluarii() {
    int completate = 0;
    for (int rand = 0; rand < ui_->questionsTable->rowCount(); ++rand) {
        const auto* raspuns = qobject_cast<QLineEdit*>(ui_->questionsTable->cellWidget(rand, 3));
        if (raspuns && !raspuns->text().trimmed().isEmpty()) ++completate;
    }
    ui_->evaluationProgressLabel->setText(
        QString::fromUtf8(u8"Progres: %1/%2")
            .arg(completate).arg(ui_->questionsTable->rowCount()));
    actualizeazaControaleleEvaluarii();
}

void StudentDashboard::finalizeazaEvaluarea() {
    if (finalizareEvaluareInCurs_) return;
    if (!poateExecutaCereri(ui_->evaluationActionStatusLabel)) {
        actualizeazaControaleleEvaluarii();
        return;
    }
    if (incercareEvaluareId_ <= 0 || incercareEvaluareFinalizata_) {
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Nu există o încercare activă care poate fi finalizată."));
        return;
    }
    if (ui_->questionsTable->rowCount() != static_cast<int>(intrebariEvaluare_.size()) ||
        intrebariEvaluare_.empty()) {
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Lista întrebărilor nu mai este validă."));
        return;
    }

    std::vector<std::pair<int, std::string>> raspunsuri;
    raspunsuri.reserve(intrebariEvaluare_.size());
    std::unordered_set<int> iduri;
    double punctajMaxim = 0.0;
    for (int rand = 0; rand < ui_->questionsTable->rowCount(); ++rand) {
        const auto* itemId = ui_->questionsTable->item(rand, 0);
        const auto* raspuns = qobject_cast<QLineEdit*>(ui_->questionsTable->cellWidget(rand, 3));
        const int intrebareId = itemId ? itemId->data(Qt::UserRole).toInt() : 0;
        const QString continut = raspuns ? raspuns->text().trimmed() : QString();
        if (intrebareId <= 0 || continut.isEmpty() || !iduri.insert(intrebareId).second ||
            intrebareId != intrebariEvaluare_[static_cast<std::size_t>(rand)].id) {
            ui_->evaluationActionStatusLabel->setText(
                QString::fromUtf8(u8"Completează o singură dată toate răspunsurile evaluării curente."));
            return;
        }
        punctajMaxim += intrebariEvaluare_[static_cast<std::size_t>(rand)].punctajMaxim;
        raspunsuri.emplace_back(intrebareId, continut.toUtf8().toStdString());
    }

    finalizareEvaluareInCurs_ = true;
    actualizeazaControaleleEvaluarii();
    ui_->evaluationActionStatusLabel->setText(QString::fromUtf8(u8"Trimitere răspunsuri..."));
    try {
        for (const auto& raspuns : raspunsuri) {
            context_->client().salveazaRaspuns(
                incercareEvaluareId_, raspuns.first, raspuns.second);
        }
        const auto rezultat = context_->client().finalizeazaIncercare(incercareEvaluareId_);
        if (rezultat.id != incercareEvaluareId_ || !rezultat.finalizataLa ||
            !std::isfinite(rezultat.scorBrut) || !std::isfinite(rezultat.notaFinala)) {
            throw ExceptieEdu("Serverul a returnat un rezultat final invalid.");
        }
        incercareEvaluareFinalizata_ = true;
        ui_->evaluationResultLabel->setText(
            QString::fromUtf8(u8"Rezultat: %1 / %2 puncte • Nota: %3 / 10")
                .arg(rezultat.scorBrut).arg(punctajMaxim).arg(rezultat.notaFinala));
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"Evaluarea a fost finalizată cu succes."));
    } catch (const ExceptieEdu& eroare) {
        ui_->evaluationActionStatusLabel->setText(
            context_->esteConectat() ? QString::fromUtf8(eroare.what())
                                     : QString::fromUtf8(u8"Conexiune pierdută."));
        actualizeazaStareConexiune();
    } catch (const std::exception&) {
        ui_->evaluationActionStatusLabel->setText(
            QString::fromUtf8(u8"A apărut o eroare neașteptată."));
    }
    finalizareEvaluareInCurs_ = false;
    actualizeazaControaleleEvaluarii();
}
