#pragma once

#include <QDialog>
#include <QString>

#include <memory>

class ApplicationContext;

namespace Ui {
class RegisterWindow;
}

class RegisterWindow final : public QDialog {
    Q_OBJECT

public:
    explicit RegisterWindow(std::shared_ptr<ApplicationContext> context,
                            QWidget* parent = nullptr);
    ~RegisterWindow() override;

    QString emailInregistrat() const;

private slots:
    void inregistreaza();
    void reconecteaza();

private:
    std::unique_ptr<Ui::RegisterWindow> ui_;
    std::shared_ptr<ApplicationContext> context_;
    QString emailInregistrat_;

    void actualizeazaStareConexiune();
    bool valideazaFormular(const QString& prenume,
                           const QString& nume,
                           const QString& email,
                           const QString& parola,
                           const QString& confirmare,
                           const QString& rol);
};
