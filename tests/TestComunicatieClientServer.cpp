#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ManagerSocket.h"
#include "ServerEdu.h"

#include <WS2tcpip.h>

#include <exception>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void verifica(bool conditie, const std::string& mesaj) {
    if (!conditie) {
        throw std::runtime_error("Verificare esuata: " + mesaj);
    }
}
}

int main() {
    const int socketuriInitiale = SocketWindows::numarSocketuriActive();
    const int instanteWinsockInitiale = InitializatorWinsock::numarInstanteActive();

    try {
        {
            ServerEdu server(0);
            server.pornesteNod();
            verifica(server.estePornit(), "serverul nu a pornit");
            verifica(server.obtinePort() != 0, "serverul nu a primit un port liber");

            std::exception_ptr eroareServer;
            std::thread firServer([&] {
                try {
                    server.proceseazaCerere();
                } catch (...) {
                    eroareServer = std::current_exception();
                }
            });

            try {
                ClientEdu client(server.obtinePort());
                client.pornesteNod();
                verifica(client.esteConectat(), "clientul nu s-a conectat");

                const std::string raspuns = client.trimiteCerere("PING");
                verifica(raspuns == "PONG", "raspunsul la PING nu este PONG");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "al doilea PING pe aceeasi conexiune a esuat");
                CerereEdu protocolInvalid;
                protocolInvalid.versiune = 999;
                protocolInvalid.tip = TipCerereEdu::Ping;
                verifica(client.executaCerere(protocolInvalid).cod ==
                             CodRezultatEdu::ProtocolInvalid,
                         "versiunea invalida nu a fost diferentiata");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "conexiunea nu a ramas activa dupa protocol invalid");
                verifica(client.trimiteCerere("CERERE_NECUNOSCUTA") ==
                             "Tip de cerere necunoscut.",
                         "cererea necunoscuta nu a fost respinsa corect");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "conexiunea nu a ramas activa dupa cererea necunoscuta");
                verifica(client.obtineUltimulRaspuns() == "PONG",
                         "clientul nu a retinut raspunsul");

                client.deconecteaza();
                verifica(!client.esteConectat(), "clientul nu s-a deconectat");
                bool cerereDupaDeconectareRespinsa = false;
                try {
                    client.trimiteCerere("PING");
                } catch (const ExceptieEdu&) {
                    cerereDupaDeconectareRespinsa = true;
                }
                verifica(cerereDupaDeconectareRespinsa,
                         "cererea dupa deconectare nu a fost respinsa");
            } catch (...) {
                server.opresteNod();
                firServer.join();
                throw;
            }

            firServer.join();
            if (eroareServer) {
                std::rethrow_exception(eroareServer);
            }

            eroareServer = nullptr;
            std::thread firDeconectare([&] {
                try {
                    server.proceseazaCerere();
                } catch (...) {
                    eroareServer = std::current_exception();
                }
            });
            {
                ClientEdu clientDeconectat(server.obtinePort());
                clientDeconectat.pornesteNod();
                clientDeconectat.opresteNod();
            }
            firDeconectare.join();
            if (eroareServer) {
                std::rethrow_exception(eroareServer);
            }

            eroareServer = nullptr;
            std::thread firMesajIncomplet([&] {
                try {
                    server.proceseazaCerere();
                } catch (...) {
                    eroareServer = std::current_exception();
                }
            });
            {
                SocketWindows socketIncomplet(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
                verifica(socketIncomplet.esteValid(),
                         "socket-ul pentru mesajul incomplet nu a fost creat");
                sockaddr_in adresa{};
                adresa.sin_family = AF_INET;
                adresa.sin_port = htons(server.obtinePort());
                InetPtonA(AF_INET, "127.0.0.1", &adresa.sin_addr);
                verifica(connect(socketIncomplet.obtine(),
                                 reinterpret_cast<const sockaddr*>(&adresa),
                                 sizeof(adresa)) != SOCKET_ERROR,
                         "clientul pentru mesajul incomplet nu s-a conectat");
                const std::uint16_t jumatatePrefix = 0;
                ManagerSocket::sendAll(socketIncomplet.obtine(),
                                       &jumatatePrefix,
                                       sizeof(jumatatePrefix));
                shutdown(socketIncomplet.obtine(), SD_BOTH);
            }
            firMesajIncomplet.join();
            verifica(static_cast<bool>(eroareServer),
                     "mesajul incomplet nu a fost tratat ca eroare de protocol");

            server.opresteNod();
            verifica(!server.estePornit(), "serverul nu s-a oprit");
        }

        {
            ServerEdu serverOpritInAccept(0);
            serverOpritInAccept.pornesteNod();
            std::exception_ptr eroareAccept;
            std::thread firAccept([&] {
                try {
                    serverOpritInAccept.proceseazaCerere();
                } catch (...) {
                    eroareAccept = std::current_exception();
                }
            });
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            serverOpritInAccept.opresteNod();
            firAccept.join();
            verifica(!eroareAccept, "oprirea serverului in accept a produs o eroare");
        }

        {
            ServerEdu serverOpritInRecv(0);
            serverOpritInRecv.pornesteNod();
            std::exception_ptr eroareRecv;
            std::thread firRecv([&] {
                try {
                    serverOpritInRecv.proceseazaCerere();
                } catch (...) {
                    eroareRecv = std::current_exception();
                }
            });
            ClientEdu clientInactiv(serverOpritInRecv.obtinePort());
            clientInactiv.pornesteNod();
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            serverOpritInRecv.opresteNod();
            firRecv.join();
            verifica(!eroareRecv, "oprirea serverului in recv a produs o eroare");
            clientInactiv.opresteNod();
        }

        verifica(SocketWindows::numarSocketuriActive() == socketuriInitiale,
                 "au ramas socket-uri deschise");
        verifica(InitializatorWinsock::numarInstanteActive() == instanteWinsockInitiale,
                 "WSAStartup si WSACleanup nu sunt echilibrate");

        std::cout << "Testul local PING/PONG a trecut.\n";
        return 0;
    } catch (const std::exception& exceptie) {
        std::cerr << exceptie.what() << '\n';
        return 1;
    }
}
