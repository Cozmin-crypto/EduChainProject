#pragma once

#include <QWidget>
#include <memory>

class ApplicationContext;
namespace Ui { class ProfessorDashboard; }

class ProfessorDashboard final : public QWidget {
    Q_OBJECT
public:
    explicit ProfessorDashboard(std::shared_ptr<ApplicationContext> context,
                                QWidget* parent = nullptr);
    ~ProfessorDashboard() override;
private:
    std::unique_ptr<Ui::ProfessorDashboard> ui_;
    std::shared_ptr<ApplicationContext> context_;
};
