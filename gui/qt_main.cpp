#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QFile>
#include <QScreen>
#include <QStyleFactory>
#include <QString>

#include "ApplicationContext.h"
#include "LoginWindow.h"

#include <charconv>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {
bool parseazaPort(std::string_view text, std::uint16_t& port) {
    unsigned int valoare{};
    const auto rezultat = std::from_chars(
        text.data(), text.data() + text.size(), valoare);
    if (rezultat.ec != std::errc{} || rezultat.ptr != text.data() + text.size() ||
        valoare == 0 || valoare > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(valoare);
    return true;
}

}

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);
    application.setStyle(QStyleFactory::create("Fusion"));
    QFile stil(":/styles/EduChainStyle.qss");
    if (stil.open(QIODevice::ReadOnly | QIODevice::Text)) {
        application.setStyleSheet(QString::fromUtf8(stil.readAll()));
    }

    if (argc > 3) {
        QMessageBox::critical(nullptr, "EduChain",
                              QString::fromUtf8(u8"Utilizare: EduChainQtClient.exe [host] [port]"));
        return 2;
    }
    const std::string host = argc >= 2 ? argv[1] : "127.0.0.1";
    std::uint16_t port = 5050;
    if (host.empty() || (argc == 3 && !parseazaPort(argv[2], port))) {
        QMessageBox::critical(nullptr, "EduChain",
                              QString::fromUtf8(u8"Host sau port invalid. Portul trebuie să fie între 1 și 65535."));
        return 2;
    }

    std::clog << "[CLIENT] Executable: "
              << QApplication::applicationFilePath().toStdString() << '\n'
              << "[CLIENT] Host: " << host << '\n'
              << "[CLIENT] Port: " << port << '\n';

    auto context = std::make_shared<ApplicationContext>(host, port);
    try {
        context->conecteaza();
    } catch (...) {
        context->deconecteaza();
    }

    LoginWindow loginWindow(context);
    loginWindow.adjustSize();
    loginWindow.show();
    if (const QScreen* ecran = loginWindow.screen()) {
        loginWindow.move(ecran->availableGeometry().center() -
                         loginWindow.rect().center());
    }

    const int rezultat = application.exec();
    context->deconecteaza();
    return rezultat;
}
