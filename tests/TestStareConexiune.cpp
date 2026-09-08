#include "ClientEdu.h"
#include "ServerEdu.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void verifica(bool conditie, const char* mesaj) {
    if (!conditie) throw std::runtime_error(mesaj);
}
}

int main() {
    try {
        ServerEdu rezervarePort(0);
        rezervarePort.pornesteNod();
        const auto portInchis = rezervarePort.obtinePort();
        rezervarePort.opresteNod();

        ClientEdu offline(portInchis);
        bool conectareRespinsa = false;
        try {
            offline.pornesteNod();
        } catch (...) {
            conectareRespinsa = true;
        }
        verifica(conectareRespinsa && !offline.esteConectat(),
                 "conectarea esuata a lasat clientul online");

        ServerEdu server(0);
        server.pornesteNod();
        ClientEdu client(server.obtinePort());
        client.pornesteNod();
        verifica(client.esteConectat(), "clientul nu s-a conectat la server");
        server.opresteNod();
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        bool cerereRespinsa = false;
        try {
            client.trimiteCerere("PING");
        } catch (...) {
            cerereRespinsa = true;
        }
        verifica(cerereRespinsa && !client.esteConectat(),
                 "pierderea serverului nu a invalidat conexiunea clientului");
        std::cout << "TestStareConexiune: PASS\n";
        return 0;
    } catch (const std::exception& exceptie) {
        std::cerr << "TestStareConexiune: FAIL: " << exceptie.what() << '\n';
        return 1;
    }
}
