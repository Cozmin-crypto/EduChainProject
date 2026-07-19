#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "ExceptieEdu.h"
#include "ManagerSocket.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
void verifica(bool conditie, const std::string& mesaj) {
    if (!conditie) {
        throw std::runtime_error("Verificare esuata: " + mesaj);
    }
}

std::filesystem::path caleTemporara() {
    const auto marcaj = std::chrono::high_resolution_clock::now()
                            .time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("educhain_retea_" + std::to_string(marcaj) + ".db");
}

void adaugaProfesor(ConectorBazaDate& conector, int id) {
    conector.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id, departament, data_angajarii) "
        "VALUES (?, ?, ?);", {std::to_string(id), "Informatica", "2026-07-19"});
    conector.executaInterogareParametrizata(
        "INSERT INTO profesori (utilizator_id) VALUES (?);", {std::to_string(id)});
}

void adaugaStudent(ConectorBazaDate& conector, int id) {
    conector.executaInterogareParametrizata(
        "INSERT INTO studenti (utilizator_id) VALUES (?);", {std::to_string(id)});
}

void adaugaAdministrator(ConectorBazaDate& conector, int id) {
    conector.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id, departament, data_angajarii) "
        "VALUES (?, ?, ?);", {std::to_string(id), "Administratie", "2026-07-19"});
    conector.executaInterogareParametrizata(
        "INSERT INTO administratori (utilizator_id, nivel_acces) VALUES (?, ?);",
        {std::to_string(id), "10"});
}

template <typename Operatie>
void ruleazaConexiune(ServerEdu& server, Operatie operatie) {
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
        operatie(client);
        if (client.esteConectat()) {
            client.deconecteaza();
        }
    } catch (...) {
        server.opresteNod();
        firServer.join();
        throw;
    }
    firServer.join();
    if (eroareServer) {
        std::rethrow_exception(eroareServer);
    }
}

CerereEdu cerereCurs(TipCerereEdu tip,
                     std::initializer_list<CampProtocolEdu> campuri = {}) {
    CerereEdu cerere;
    cerere.tip = tip;
    cerere.campuri = campuri;
    return cerere;
}
}

int main() {
    const auto cale = caleTemporara();
    const int socketuriInitiale = SocketWindows::numarSocketuriActive();
    const int winsockInitial = InitializatorWinsock::numarInstanteActive();
    ConectorBazaDate conector;
    try {
        std::filesystem::remove(cale);
        conector.deschideConexiune(cale.string());
        UtilizatorRepository utilizatori(conector);
        CursRepository cursuriRepository(conector);
        AutentificareService autentificare(utilizatori);
        CursService cursuri(cursuriRepository, utilizatori);

        const int profesor1 = utilizatori.adaugaUtilizator(
            "profesor1.retea@example.ro", "parola-1", "profesor");
        const int profesor2 = utilizatori.adaugaUtilizator(
            "profesor2.retea@example.ro", "parola-2", "profesor");
        const int student = utilizatori.adaugaUtilizator(
            "student.retea@example.ro", "parola-student", "student");
        const int administrator = utilizatori.adaugaUtilizator(
            "admin.retea@example.ro", "parola-admin", "administrator");
        adaugaProfesor(conector, profesor1);
        adaugaProfesor(conector, profesor2);
        adaugaStudent(conector, student);
        adaugaAdministrator(conector, administrator);

        const int cursProfesor1 = cursuri.creeazaCurs(
            {profesor1, profesor1, "Curs initial profesor 1", std::nullopt});
        const int cursProfesor2 = cursuri.creeazaCurs(
            {profesor2, profesor2, "Curs initial profesor 2", std::nullopt});

        {
            ServerEdu server(autentificare, cursuri, 0);
            server.pornesteNod();

            ruleazaConexiune(server, [&](ClientEdu& client) {
                const auto neautentificat = client.executaCerere(
                    cerereCurs(TipCerereEdu::ListeazaCursuri));
                verifica(neautentificat.cod == CodRezultatEdu::AccesInterzis,
                         "cererea fara autentificare nu a fost respinsa");

                const auto emailInexistent = client.autentifica(
                    "absent@example.ro", "parola");
                const auto parolaGresita = client.autentifica(
                    "student.retea@example.ro", "gresita");
                verifica(emailInexistent.cod == CodRezultatEdu::AutentificareEsuata &&
                             parolaGresita.cod == CodRezultatEdu::AutentificareEsuata &&
                             emailInexistent.mesajPublic == parolaGresita.mesajPublic,
                         "esecurile autentificarii expun informatii diferite");
                verifica(client.esteConectat() && !client.esteAutentificat(),
                         "login-ul esuat a inchis conexiunea sau a creat sesiune");

                const auto succes = client.autentifica(
                    "student.retea@example.ro", "parola-student");
                verifica(succes.cod == CodRezultatEdu::Succes &&
                             client.esteAutentificat() &&
                             client.obtineRolAutentificat() == "student",
                         "login-ul studentului a esuat");
                const auto alDoileaLogin = client.autentifica(
                    "profesor1.retea@example.ro", "parola-1");
                verifica(alDoileaLogin.cod == CodRezultatEdu::AccesInterzis &&
                             client.obtineRolAutentificat() == "student",
                         "al doilea login a inlocuit sesiunea");

                verifica(!client.listeazaCursuri().empty(),
                         "studentul nu poate lista cursurile");
                verifica(client.obtineCurs(cursProfesor1).has_value(),
                         "studentul nu poate obtine un curs");
                verifica(!client.obtineCurs(999999).has_value(),
                         "cursul inexistent nu este raportat corect");

                auto injectare = cerereCurs(
                    TipCerereEdu::CreeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::Nume), "Injectat"},
                     {static_cast<std::uint16_t>(CampEdu::UtilizatorId),
                      std::to_string(profesor1)},
                     {static_cast<std::uint16_t>(CampEdu::Rol), "profesor"}});
                const auto injectareRespinsa = client.executaCerere(std::move(injectare));
                verifica(injectareRespinsa.cod == CodRezultatEdu::ValidareEsuata,
                         "clientul a putut declara actorId sau rol");

                const auto creareStudent = client.executaCerere(cerereCurs(
                    TipCerereEdu::CreeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::Nume), "Curs student"}}));
                const auto actualizareStudent = client.executaCerere(cerereCurs(
                    TipCerereEdu::ActualizeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::CursId),
                      std::to_string(cursProfesor1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Modificat"}}));
                const auto stergereStudent = client.executaCerere(cerereCurs(
                    TipCerereEdu::StergeCurs,
                    {{static_cast<std::uint16_t>(CampEdu::CursId),
                      std::to_string(cursProfesor1)}}));
                verifica(creareStudent.cod == CodRezultatEdu::AccesInterzis &&
                             actualizareStudent.cod == CodRezultatEdu::AccesInterzis &&
                             stergereStudent.cod == CodRezultatEdu::AccesInterzis,
                         "studentul a primit drepturi de administrare");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "conexiunea nu a ramas activa dupa erori de business");
            });

            int cursCreatPrinRetea{};
            ruleazaConexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "profesor1.retea@example.ro", "parola-1").cod ==
                             CodRezultatEdu::Succes,
                         "login-ul profesorului a esuat");
                cursCreatPrinRetea = client.creeazaCurs(
                    "Curs cu text '); DROP TABLE cursuri; --");
                verifica(client.obtineCurs(cursCreatPrinRetea)->proprietarId == profesor1,
                         "profesorul nu a devenit proprietarul cursului sau");
                client.actualizeazaCurs(cursCreatPrinRetea, "Curs actualizat prin retea");
                verifica(client.obtineCurs(cursCreatPrinRetea)->nume ==
                             "Curs actualizat prin retea",
                         "actualizarea cursului propriu a esuat");

                const auto modificareStraina = client.executaCerere(cerereCurs(
                    TipCerereEdu::ActualizeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::CursId),
                      std::to_string(cursProfesor2)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Interzis"}}));
                const auto stergereStraina = client.executaCerere(cerereCurs(
                    TipCerereEdu::StergeCurs,
                    {{static_cast<std::uint16_t>(CampEdu::CursId),
                      std::to_string(cursProfesor2)}}));
                verifica(modificareStraina.cod == CodRezultatEdu::AccesInterzis &&
                             stergereStraina.cod == CodRezultatEdu::AccesInterzis,
                         "profesorul a administrat cursul strain");
                verifica(cursuri.obtineCurs(cursProfesor2)->nume ==
                             "Curs initial profesor 2",
                         "operatia interzisa a modificat datele");

                const auto invalid = client.executaCerere(cerereCurs(
                    TipCerereEdu::CreeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::Nume), ""}}));
                verifica(invalid.cod == CodRezultatEdu::ValidareEsuata,
                         "datele invalide nu au fost respinse");

                const int parinte = client.creeazaCurs("Parinte conflict");
                client.creeazaCurs("Copil duplicat", parinte);
                const auto numarInainte = cursuri.listeazaCursuri().size();
                const auto conflict = client.executaCerere(cerereCurs(
                    TipCerereEdu::CreeazaCurs,
                    {{static_cast<std::uint16_t>(CampEdu::Nume), "Copil duplicat"},
                     {static_cast<std::uint16_t>(CampEdu::ParinteId),
                      std::to_string(parinte)}}));
                verifica(conflict.cod == CodRezultatEdu::Conflict,
                         "conflictul UNIQUE nu a fost diferentiat");
                verifica(cursuri.listeazaCursuri().size() == numarInainte,
                         "conflictul a lasat o operatie partiala");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "conexiunea s-a inchis dupa conflict");
            });

            ruleazaConexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "profesor2.retea@example.ro", "parola-2").cod ==
                             CodRezultatEdu::Succes,
                         "login-ul celui de-al doilea profesor a esuat");
                verifica(!client.listeazaCursuri().empty(),
                         "profesorul nu poate lista cursurile");
            });

            ruleazaConexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "admin.retea@example.ro", "parola-admin").cod ==
                             CodRezultatEdu::Succes,
                         "login-ul administratorului a esuat");
                const int cursAdmin = client.creeazaCurs(
                    "Curs creat de administrator", std::nullopt, profesor2);
                verifica(client.obtineCurs(cursAdmin)->proprietarId == profesor2 &&
                             client.obtineCurs(cursAdmin)->proprietarId != administrator,
                         "administratorul a devenit proprietar");
                client.actualizeazaCurs(cursProfesor1, "Modificat de administrator");
                client.stergeCurs(cursAdmin);
                verifica(!client.obtineCurs(cursAdmin).has_value(),
                         "administratorul nu a sters cursul");
            });

            server.opresteNod();
        }

        conector.inchideConexiune();
        verifica(std::filesystem::remove(cale), "baza temporara nu a fost eliminata");
        verifica(SocketWindows::numarSocketuriActive() == socketuriInitiale,
                 "au ramas socket-uri active");
        verifica(InitializatorWinsock::numarInstanteActive() == winsockInitial,
                 "WSAStartup si WSACleanup nu sunt echilibrate");
        std::cout << "Testul de integrare retea-servicii a trecut.\n";
        return 0;
    } catch (const std::exception& exceptie) {
        if (conector.esteConectat()) {
            try {
                conector.inchideConexiune();
            } catch (...) {
            }
        }
        std::error_code eroare;
        std::filesystem::remove(cale, eroare);
        std::cerr << exceptie.what() << '\n';
        return 1;
    }
}
