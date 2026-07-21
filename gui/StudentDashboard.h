#pragma once

#include "ProtocolEdu.h"

#include <QWidget>
#include <memory>
#include <vector>

class ApplicationContext;
namespace Ui { class StudentDashboard; }

class StudentDashboard final : public QWidget {
    Q_OBJECT
public:
    explicit StudentDashboard(std::shared_ptr<ApplicationContext> context,
                              QWidget* parent = nullptr);
    ~StudentDashboard() override;
private slots:
    void incarcaCursurileMele();
    void incarcaCursurileDisponibile();
    void inscrieLaCurs();
    void actualizeazaSelectiaCursului();
    void incarcaCursurilePentruLectii();
    void incarcaLectiileCursului();
    void afiseazaLectiaSelectata();
    void incarcaCursurilePentruEvaluari();
    void incarcaEvaluarileCursului();
    void afiseazaEvaluareaSelectata();
    void pornesteEvaluarea();
    void actualizeazaProgresulEvaluarii();
    void finalizeazaEvaluarea();
private:
    std::unique_ptr<Ui::StudentDashboard> ui_;
    std::shared_ptr<ApplicationContext> context_;
    std::vector<LectiePublicEdu> lectiiCurente_;
    std::vector<IntrebarePublicEdu> intrebariEvaluare_;
    int incercareEvaluareId_{};
    bool incercareEvaluareFinalizata_{};
    bool finalizareEvaluareInCurs_{};

    bool poateExecutaCereri(class QLabel* statusLabel);
    void actualizeazaStareConexiune();
    void golesteDetaliileLectiei();
    void actualizeazaControaleleLectiilor(bool cerereInCurs = false);
    void golesteEvaluarea();
    void actualizeazaControaleleEvaluarii(bool cerereInCurs = false);
    void incarcaEvaluarileCursuluiCuSelectie(int evaluarePreferata);
};
