#pragma once

#include <QWidget>
#include <memory>

class ApplicationContext;
namespace Ui { class AdminDashboard; }

class AdminDashboard final : public QWidget {
    Q_OBJECT
public:
    explicit AdminDashboard(std::shared_ptr<ApplicationContext> context,
                            QWidget* parent = nullptr);
    ~AdminDashboard() override;
private:
    std::unique_ptr<Ui::AdminDashboard> ui_;
    std::shared_ptr<ApplicationContext> context_;
};
