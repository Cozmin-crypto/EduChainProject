#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "ManagerSocket.h"
#include "ProtocolEdu.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void verifica(bool conditie, const char* mesaj) {
    if (!conditie) throw std::runtime_error(mesaj);
}

template <typename Operatie>
void conexiune(ServerEdu& server, Operatie operatie) {
    std::exception_ptr eroareServer;
    std::thread fir([&] { try { server.proceseazaCerere(); }
                         catch (...) { eroareServer = std::current_exception(); } });
    try {
        ClientEdu client(server.obtinePort());
        client.pornesteNod();
        operatie(client);
        if (client.esteConectat()) client.deconecteaza();
    } catch (...) {
        server.opresteNod();
        fir.join();
        throw;
    }
    fir.join();
    if (eroareServer) std::rethrow_exception(eroareServer);
}
}

int main() {
    const auto cale = std::filesystem::temp_directory_path() /
        ("educhain_securitate_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".db");
    const int socketuri = SocketWindows::numarSocketuriActive();
    const int winsock = InitializatorWinsock::numarInstanteActive();
    ConectorBazaDate db;
    try {
        db.deschideConexiune(cale.string());
        UtilizatorRepository utilizatori(db);
        CursRepository cursuriRepository(db);
        CursService cursuri(cursuriRepository, utilizatori);
        AutentificareService autentificare(utilizatori);

        const int legacyStudent = utilizatori.adaugaUtilizator(
            "legacy.student@example.ro", "legacy-pass", "student",
            "Legacy", "Student");
        db.executaInterogareParametrizata(
            "INSERT INTO studenti(utilizator_id) VALUES (?);",
            {std::to_string(legacyStudent)});

        db.executaInterogare("PRAGMA ignore_check_constraints = ON;");
        db.executaInterogareParametrizata(
            "INSERT INTO utilizatori(nume,prenume,email,parola,rol) VALUES (?,?,?,?,?);",
            {"Legacy", "Admin", "legacy.admin@example.ro", "admin-pass", "administrator"});
        db.executaInterogare("PRAGMA ignore_check_constraints = OFF;");

        int contNou{};
        {
            ServerEdu server(autentificare, cursuri, 0);
            server.pornesteNod();
            conexiune(server, [&](ClientEdu& client) {
                contNou = client.inregistreaza(
                    "Popescu", "Ana", "ana.secure@example.ro",
                    "parola-sigura", "student");
                verifica(contNou > 0, "inregistrarea contului hash-uit a esuat");
            });

            const auto nou = utilizatori.cautaDupaId(contNou);
            verifica(nou && nou->parola != "parola-sigura" &&
                         nou->parola.rfind("$pbkdf2-sha256$600000$", 0) == 0,
                     "parola noua este stocata in clar sau in format invalid");

            conexiune(server, [&](ClientEdu& client) {
                const auto login = client.autentifica(
                    "ana.secure@example.ro", "parola-sigura");
                verifica(login.cod == CodRezultatEdu::Succes &&
                             client.obtineRolAutentificat() == "student",
                         "login-ul cu hash valid a esuat");
                verifica(!ProtocolEdu::cautaCamp(login.campuri, CampEdu::Parola),
                         "hash-ul a fost trimis clientului");
            });
            conexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "ana.secure@example.ro", "parola-gresita").cod ==
                             CodRezultatEdu::AutentificareEsuata,
                         "parola gresita a fost acceptata");
            });
            conexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "legacy.student@example.ro", "legacy-pass").cod ==
                             CodRezultatEdu::Succes,
                         "login-ul contului legacy a esuat");
            });
            const auto migrat = utilizatori.cautaDupaId(legacyStudent);
            verifica(migrat && migrat->parola != "legacy-pass" &&
                         migrat->parola.rfind("$pbkdf2-sha256$", 0) == 0,
                     "parola legacy nu a fost migrata");

            conexiune(server, [&](ClientEdu& client) {
                const auto admin = client.autentifica(
                    "legacy.admin@example.ro", "admin-pass");
                verifica(admin.cod == CodRezultatEdu::AutentificareEsuata &&
                             !client.esteAutentificat(),
                         "rolul administrator vechi este acceptat");
            });
            conexiune(server, [&](ClientEdu& client) {
                CerereEdu cerere;
                cerere.tip = TipCerereEdu::Inregistrare;
                cerere.campuri = {
                    {static_cast<std::uint16_t>(CampEdu::Nume), "Admin"},
                    {static_cast<std::uint16_t>(CampEdu::Prenume), "Nou"},
                    {static_cast<std::uint16_t>(CampEdu::Email), "admin.nou@example.ro"},
                    {static_cast<std::uint16_t>(CampEdu::Parola), "parola-admin"},
                    {static_cast<std::uint16_t>(CampEdu::Rol), "administrator"}};
                verifica(client.executaCerere(cerere).cod == CodRezultatEdu::ValidareEsuata,
                         "inregistrarea rolului administrator este acceptata");
            });
            server.opresteNod();
        }

        db.inchideConexiune();
        std::filesystem::remove(cale);
        verifica(SocketWindows::numarSocketuriActive() == socketuri,
                 "au ramas socketuri active");
        verifica(InitializatorWinsock::numarInstanteActive() == winsock,
                 "au ramas instante Winsock active");
        std::cout << "Test securitate autentificare: SUCCES\n";
        return 0;
    } catch (const std::exception& eroare) {
        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cerr << eroare.what() << '\n';
        return 1;
    }
}
