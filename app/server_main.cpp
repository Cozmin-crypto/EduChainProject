#include "AutentificareService.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "EvaluareRepository.h"
#include "EvaluareService.h"
#include "ExceptieEdu.h"
#include "InscriereRepository.h"
#include "InscriereService.h"
#include "LectieRepository.h"
#include "LectieService.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"

#include <Windows.h>

#include <atomic>
#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <string_view>

namespace {

constexpr std::uint16_t portImplicit = 5050;
constexpr const char* caleBazaDateImplicita = "educhain.db";

std::atomic<bool> continuaRularea{true};
std::atomic<ServerEdu*> serverActiv{nullptr};

bool parseazaPort(std::string_view text, std::uint16_t& port) {
    unsigned int valoare{};
    const auto rezultat = std::from_chars(text.data(), text.data() + text.size(), valoare);
    if (rezultat.ec != std::errc{} || rezultat.ptr != text.data() + text.size() ||
        valoare == 0 || valoare > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(valoare);
    return true;
}

BOOL WINAPI gestioneazaControlConsola(DWORD tipControl) {
    if (tipControl != CTRL_C_EVENT && tipControl != CTRL_CLOSE_EVENT &&
        tipControl != CTRL_BREAK_EVENT) {
        return FALSE;
    }

    continuaRularea = false;
    if (ServerEdu* server = serverActiv.load()) {
        server->opresteNod();
    }
    return TRUE;
}

void afiseazaUtilizare() {
    std::cerr << "Utilizare: EduChainServer.exe [port] [cale_baza_date]\n";
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 3) {
        afiseazaUtilizare();
        return 2;
    }

    std::uint16_t portCerut = portImplicit;
    if (argc >= 2 && !parseazaPort(argv[1], portCerut)) {
        std::cerr << "Port invalid. Foloseste un numar intre 1 si 65535.\n";
        return 2;
    }

    const std::string caleBazaDate = argc == 3 ? argv[2] : caleBazaDateImplicita;
    if (caleBazaDate.empty()) {
        std::cerr << "Calea bazei de date nu poate fi goala.\n";
        return 2;
    }

    if (!SetConsoleCtrlHandler(gestioneazaControlConsola, TRUE)) {
        std::cerr << "Nu s-a putut instala handlerul pentru oprire controlata.\n";
        return 1;
    }

    int codIesire = 0;
    try {
        ConectorBazaDate bazaDate;
        bazaDate.deschideConexiune(caleBazaDate);

        UtilizatorRepository utilizatori(bazaDate);
        CursRepository cursuriRepository(bazaDate);
        LectieRepository lectiiRepository(bazaDate);
        EvaluareRepository evaluariRepository(bazaDate);
        InscriereRepository inscrieriRepository(bazaDate);
        AutentificareService autentificare(utilizatori);
        InscriereService inscrieri(inscrieriRepository, cursuriRepository, utilizatori);
        CursService cursuri(cursuriRepository, utilizatori, inscrieri);
        LectieService lectii(lectiiRepository, cursuriRepository, utilizatori, inscrieri);
        EvaluareService evaluari(evaluariRepository, cursuriRepository, utilizatori, inscrieri);

        {
            ServerEdu server(autentificare, cursuri, lectii, evaluari, inscrieri, portCerut);
            serverActiv = &server;
            server.pornesteNod();

            std::cout << "EduChainServer pornit\n"
                      << "Adresa: 127.0.0.1\n"
                      << "Port cerut: " << portCerut << "\n"
                      << "Port efectiv: " << server.obtinePort() << "\n"
                      << "Baza de date: " << caleBazaDate << "\n"
                      << "Apasa Ctrl+C pentru oprire\n";

            while (continuaRularea && server.estePornit()) {
                try {
                    server.proceseazaCerere();
                } catch (const ExceptieEdu& exceptie) {
                    if (continuaRularea && server.estePornit()) {
                        std::cerr << "Conexiune inchisa cu eroare: " << exceptie.what() << '\n';
                    }
                }
            }

            server.opresteNod();
            serverActiv = nullptr;
        }

        bazaDate.inchideConexiune();
        std::cout << "EduChainServer oprit controlat.\n";
    } catch (const ExceptieEdu& exceptie) {
        serverActiv = nullptr;
        std::cerr << "Eroare EduChainServer: " << exceptie.what() << '\n';
        codIesire = 1;
    } catch (const std::exception& exceptie) {
        serverActiv = nullptr;
        std::cerr << "Eroare standard: " << exceptie.what() << '\n';
        codIesire = 1;
    } catch (...) {
        serverActiv = nullptr;
        std::cerr << "Eroare necunoscuta in EduChainServer.\n";
        codIesire = 1;
    }

    SetConsoleCtrlHandler(gestioneazaControlConsola, FALSE);
    return codIesire;
}
