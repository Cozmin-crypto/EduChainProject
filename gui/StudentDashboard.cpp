#include "StudentDashboard.h"
#include "ApplicationContext.h"
#include "ui_StudentDashboard.h"

#include <QPushButton>
#include <QStackedWidget>
#include <QString>

StudentDashboard::StudentDashboard(std::shared_ptr<ApplicationContext> context,
                                   QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::StudentDashboard>()),
      context_(std::move(context)) {
    ui_->setupUi(this);
    ui_->userLabel->setText(QString::fromUtf8("Utilizator: %1")
                                .arg(QString::fromStdString(context_->email())));
    ui_->roleLabel->setText(QString::fromUtf8("Rol: %1")
                                .arg(QString::fromStdString(context_->rol())));
    ui_->connectionLabel->setText(
        context_->esteConectat()
            ? QString::fromUtf8("Conectat la %1:%2")
                  .arg(QString::fromStdString(context_->host())).arg(context_->port())
            : QString::fromUtf8(u8"Conexiune pierdută"));
    connect(ui_->homeButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(0); });
    connect(ui_->myCoursesButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(1); });
    connect(ui_->availableCoursesButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(2); });
    connect(ui_->lessonsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(3); });
    connect(ui_->evaluationsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(4); });
    connect(ui_->resultsButton, &QPushButton::clicked, ui_->pagesStack,
            [this] { ui_->pagesStack->setCurrentIndex(5); });
}

StudentDashboard::~StudentDashboard() = default;
