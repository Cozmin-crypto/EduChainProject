#pragma once

#include <QWidget>

#include <memory>

namespace Ui {
class LoginWindow;
}

class LoginWindow final : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget* parent = nullptr);
    ~LoginWindow() override;

private slots:
    void afiseazaMesajLoginNeimplementat();
    void afiseazaMesajRegisterNeimplementat();

private:
    std::unique_ptr<Ui::LoginWindow> ui_;
};
