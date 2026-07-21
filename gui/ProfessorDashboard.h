#pragma once

#include <QWidget>

#include <memory>

class ApplicationContext;
class QLabel;
namespace Ui { class ProfessorDashboard; }

class ProfessorDashboard final : public QWidget {
    Q_OBJECT
public:
    explicit ProfessorDashboard(std::shared_ptr<ApplicationContext> context,
                                QWidget* parent = nullptr);
    ~ProfessorDashboard() override;

private slots:
    void incarcaCursurileAcasa();
    void creeazaCurs();
    void incarcaCursurilePentruStudenti();
    void incarcaStudentiiCursului();
    void inscrieStudentLaCurs();
    void incarcaCursurilePentruLectii();
    void incarcaLectiileCursului();
    void afiseazaLectiaSelectata();
    void actualizeazaCampurileTipului();
    void adaugaLectie();
    void incarcaCursurilePentruEvaluari();
    void incarcaEvaluarileCursului();
    void afiseazaEvaluareaSelectata();
    void actualizeazaCampurileTipuluiEvaluarii();
    void creeazaEvaluare();
    void adaugaIntrebare();

private:
    std::unique_ptr<Ui::ProfessorDashboard> ui_;
    std::shared_ptr<ApplicationContext> context_;

    bool poateExecutaCereri(QLabel* statusLabel);
    void actualizeazaStareConexiune();
    void actualizeazaControaleleAdministrarii(bool cerereInCurs = false);
    void incarcaCursurileAcasaCuSelectie(int cursPreferat);
    void incarcaStudentiiCursuluiCuSelectie(int studentPreferat);
    void actualizeazaControalele(bool cerereInCurs = false);
    void golesteDetaliileLectiei();
    void golesteFormularul();
    void incarcaLectiileCursuluiCuSelectie(int lectiePreferata);
    void actualizeazaControaleleEvaluarilor(bool cerereInCurs = false);
    void golesteDetaliileEvaluarii();
    void golesteFormularulEvaluarii();
    void golesteFormularulIntrebarii();
    void incarcaEvaluarileCursuluiCuSelectie(int evaluarePreferata);
};
