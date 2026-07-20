#include <QApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

#include "ApplicationContext.h"
#include "LoginWindow.h"

#include <charconv>
#include <cstdint>
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

bool solicitaReincercareConectare() {
    QMessageBox mesaj;
    mesaj.setIcon(QMessageBox::Critical);
    mesaj.setWindowTitle("EduChain");
    mesaj.setText(QString::fromUtf8(u8"Nu s-a putut realiza conexiunea la server."));
    auto* retryButton = mesaj.addButton("Retry", QMessageBox::AcceptRole);
    mesaj.addButton("Exit", QMessageBox::RejectRole);
    mesaj.setDefaultButton(retryButton);
    mesaj.exec();
    return mesaj.clickedButton() == retryButton;
}
}

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);

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

    auto context = std::make_shared<ApplicationContext>(host, port);
    bool continua = true;
    while (continua && !context->esteConectat()) {
        try {
            context->conecteaza();
        } catch (...) {
            continua = solicitaReincercareConectare();
        }
    }
    if (!continua) {
        context->deconecteaza();
        return 1;
    }

    LoginWindow loginWindow(context);
    loginWindow.show();

    const int rezultat = application.exec();
    context->deconecteaza();
    return rezultat;
}
