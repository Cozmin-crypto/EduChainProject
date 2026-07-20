#pragma once

#include <QWidget>
#include <memory>

class ApplicationContext;
namespace Ui { class StudentDashboard; }

class StudentDashboard final : public QWidget {
    Q_OBJECT
public:
    explicit StudentDashboard(std::shared_ptr<ApplicationContext> context,
                              QWidget* parent = nullptr);
    ~StudentDashboard() override;
private:
    std::unique_ptr<Ui::StudentDashboard> ui_;
    std::shared_ptr<ApplicationContext> context_;
};
