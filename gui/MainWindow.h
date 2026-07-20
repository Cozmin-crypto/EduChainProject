#pragma once

#include <QMainWindow>

#include <memory>

class ApplicationContext;
namespace Ui { class MainWindow; }

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(std::shared_ptr<ApplicationContext> context,
                        QWidget* parent = nullptr);
    ~MainWindow() override;
private slots:
    void logout();
    void inchideAplicatia();
private:
    std::unique_ptr<Ui::MainWindow> ui_;
    std::shared_ptr<ApplicationContext> context_;
};
