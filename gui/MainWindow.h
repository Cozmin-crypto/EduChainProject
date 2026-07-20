#pragma once

#include <QMainWindow>

#include <memory>

class ApplicationContext;
class QCloseEvent;
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
    bool inchiderePentruSchimbareFereastra_{};

    void configureazaDashboard();
    bool reconecteazaCuDialog();
    void revinoLaLogin();
    void trateazaRolNecunoscut();
    void closeEvent(QCloseEvent* event) override;
};
