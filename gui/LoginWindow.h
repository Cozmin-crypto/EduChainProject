#pragma once

#include <QWidget>

#include <memory>

class ApplicationContext;

namespace Ui {
class LoginWindow;
}

class LoginWindow final : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(std::shared_ptr<ApplicationContext> context,
                         QWidget* parent = nullptr);
    ~LoginWindow() override;

private slots:
    void autentifica();
    void deschideInregistrare();
    void reconecteaza();

private:
    std::unique_ptr<Ui::LoginWindow> ui_;
    std::shared_ptr<ApplicationContext> context_;

    void actualizeazaStareConexiune();
};
