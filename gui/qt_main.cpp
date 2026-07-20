#include <QApplication>

#include "LoginWindow.h"

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);

    LoginWindow loginWindow;
    loginWindow.show();

    return application.exec();
}
