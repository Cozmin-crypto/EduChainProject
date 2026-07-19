#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "ExceptieEdu.h"
#include "LectieRepository.h"
#include "LectieService.h"
#include "ManagerSocket.h"
#include "ProtocolEdu.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"

#include <WinSock2.h>

#include <chrono>
#include <cstring>
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

bool aruncaExceptieEdu(const std::function<void()>& operatie) {
    try {
        operatie();
    } catch (const ExceptieEdu&) {
        return true;
    }
    return false;
}

std::filesystem::path caleTemporara() {
    const auto marcaj = std::chrono::high_resolution_clock::now()
                            .time_since_epoch().count();
    return std::filesystem::temp_directory_path() /
           ("educhain_lectii_retea_" + std::to_string(marcaj) + ".db");
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

CerereEdu cerere(TipCerereEdu tip,
                 std::initializer_list<CampProtocolEdu> campuri = {}) {
    CerereEdu rezultat;
    rezultat.tip = tip;
    rezultat.campuri = campuri;
    return rezultat;
}

void verificaDecodareStricta() {
    verifica(aruncaExceptieEdu([] {
                 ProtocolEdu::decodificaCerere(std::string(3, '\0'));
             }), "mesajul trunchiat a fost acceptat");

    CerereEdu valida;
    valida.tip = TipCerereEdu::ObtineLectie;
    valida.idCerere = 7;
    valida.campuri = {{static_cast<std::uint16_t>(CampEdu::LectieId), "1"}};
    std::string lungimeInvalida = ProtocolEdu::codificaCerere(valida);
    const std::uint32_t lungimeMare = htonl(1000);
    constexpr std::size_t pozitieLungimeCamp = 12;
    std::memcpy(lungimeInvalida.data() + pozitieLungimeCamp,
                &lungimeMare, sizeof(lungimeMare));
    verifica(aruncaExceptieEdu([&] {
                 ProtocolEdu::decodificaCerere(lungimeInvalida);
             }), "lungimea mai mare decat payload-ul a fost acceptata");

    std::string numarCampuriInvalid = ProtocolEdu::codificaCerere(valida);
    const std::uint16_t preaMulte = htons(100);
    constexpr std::size_t pozitieNumarCampuri = 8;
    std::memcpy(numarCampuriInvalid.data() + pozitieNumarCampuri,
                &preaMulte, sizeof(preaMulte));
    verifica(aruncaExceptieEdu([&] {
                 ProtocolEdu::decodificaCerere(numarCampuriInvalid);
             }), "numarul imposibil de campuri a fost acceptat");
}
}

int main() {
    const auto cale = caleTemporara();
    const int socketuriInitiale = SocketWindows::numarSocketuriActive();
    const int winsockInitial = InitializatorWinsock::numarInstanteActive();
    ConectorBazaDate conector;
    try {
        verificaDecodareStricta();
        std::filesystem::remove(cale);
        conector.deschideConexiune(cale.string());
        UtilizatorRepository utilizatori(conector);
        CursRepository cursuriRepository(conector);
        LectieRepository lectiiRepository(conector);
        AutentificareService autentificare(utilizatori);
        CursService cursuri(cursuriRepository, utilizatori);
        LectieService lectii(lectiiRepository, cursuriRepository, utilizatori);

        const int profesor1 = utilizatori.adaugaUtilizator(
            "profesor.lectii@example.ro", "parola-1", "profesor");
        const int profesor2 = utilizatori.adaugaUtilizator(
            "profesor2.lectii@example.ro", "parola-2", "profesor");
        const int student = utilizatori.adaugaUtilizator(
            "student.lectii@example.ro", "parola-student", "student");
        const int administrator = utilizatori.adaugaUtilizator(
            "admin.lectii@example.ro", "parola-admin", "administrator");
        adaugaProfesor(conector, profesor1);
        adaugaProfesor(conector, profesor2);
        adaugaStudent(conector, student);
        adaugaAdministrator(conector, administrator);

        const int curs1 = cursuri.creeazaCurs(
            {profesor1, profesor1, "Curs lectii 1", std::nullopt});
        const int curs2 = cursuri.creeazaCurs(
            {profesor2, profesor2, "Curs lectii 2", std::nullopt});
        const int textInitial = lectii.creeazaLectie(
            {profesor1, curs1, "Text initial", "text", "continut", 8, 1,
             std::nullopt, std::nullopt});
        const int videoInitial = lectii.creeazaLectie(
            {profesor1, curs1, "Video initial", "video", "video.mp4", 1024,
             std::nullopt, 60, std::string("h264")});

        {
            ServerEdu server(autentificare, cursuri, lectii, 0);
            server.pornesteNod();

            ruleazaConexiune(server, [&](ClientEdu& client) {
                const auto faraLogin = client.executaCerere(cerere(
                    TipCerereEdu::ListeazaLectii,
                    {{static_cast<std::uint16_t>(CampEdu::CursId),
                      std::to_string(curs1)}}));
                verifica(faraLogin.cod == CodRezultatEdu::AccesInterzis,
                         "listarea fara autentificare nu a fost respinsa");
            });

            ruleazaConexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "student.lectii@example.ro", "parola-student").cod ==
                             CodRezultatEdu::Succes,
                         "autentificarea studentului a esuat");
                const auto lista = client.listeazaLectii(curs1);
                verifica(lista.size() == 2 && lista[0].id == textInitial &&
                             lista[1].id == videoInitial,
                         "ordinea lectiilor este incorecta");
                verifica(client.obtineLectie(textInitial)->tip == TipLectieEdu::Text,
                         "lectia text nu a fost citita");
                verifica(client.obtineLectie(videoInitial)->tip == TipLectieEdu::Video,
                         "lectia video nu a fost citita");
                verifica(!client.obtineLectie(999999).has_value(),
                         "lectia inexistenta nu a fost raportata");

                const auto cursInexistent = client.executaCerere(cerere(
                    TipCerereEdu::ListeazaLectii,
                    {{static_cast<std::uint16_t>(CampEdu::CursId), "999999"}}));
                verifica(cursInexistent.cod == CodRezultatEdu::ResursaInexistenta,
                         "cursul inexistent nu a fost raportat");
                const auto creareStudent = client.executaCerere(cerere(
                    TipCerereEdu::CreeazaLectieText,
                    {{static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Interzisa"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"},
                     {static_cast<std::uint16_t>(CampEdu::NumarCuvinte), "1"}}));
                verifica(creareStudent.cod == CodRezultatEdu::AccesInterzis,
                         "studentul a creat o lectie");
                const auto actualizareStudent = client.executaCerere(cerere(
                    TipCerereEdu::ActualizeazaLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(textInitial)},
                     {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Interzisa"},
                     {static_cast<std::uint16_t>(CampEdu::TipLectie), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"},
                     {static_cast<std::uint16_t>(CampEdu::NumarCuvinte), "1"}}));
                const auto stergereStudent = client.executaCerere(cerere(
                    TipCerereEdu::StergeLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(textInitial)}}));
                verifica(actualizareStudent.cod == CodRezultatEdu::AccesInterzis &&
                             stergereStudent.cod == CodRezultatEdu::AccesInterzis &&
                             client.obtineLectie(textInitial).has_value(),
                         "studentul a actualizat sau a sters o lectie");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "conexiunea nu a ramas activa dupa refuz");
            });

            int lectieText{};
            int lectieVideo{};
            ruleazaConexiune(server, [&](ClientEdu& client) {
                verifica(client.autentifica(
                             "profesor.lectii@example.ro", "parola-1").cod ==
                             CodRezultatEdu::Succes,
                         "autentificarea profesorului proprietar a esuat");
                lectieText = client.creeazaLectieText(
                    curs1, "Text SQL", "Text '); DROP TABLE lectii; --", 32, 5);
                lectieVideo = client.creeazaLectieVideo(
                    curs1, "Video retea", "fisier.mp4", 2048, 120, "vp9");
                verifica(client.obtineLectie(lectieText)->continut ==
                             "Text '); DROP TABLE lectii; --",
                         "caracterele SQL nu au fost tratate literal");

                const auto lipsaCamp = client.executaCerere(cerere(
                    TipCerereEdu::CreeazaLectieText,
                    {{static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)}}));
                const auto duplicat = client.executaCerere(cerere(
                    TipCerereEdu::ObtineLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId), "1"},
                     {static_cast<std::uint16_t>(CampEdu::LectieId), "2"}}));
                const auto necunoscut = client.executaCerere(cerere(
                    TipCerereEdu::ObtineLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId), "1"},
                     {65000, "x"}}));
                const auto numarInvalid = client.executaCerere(cerere(
                    TipCerereEdu::ObtineLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId), "12abc"}}));
                verifica(lipsaCamp.cod == CodRezultatEdu::ValidareEsuata &&
                             duplicat.cod == CodRezultatEdu::ValidareEsuata &&
                             necunoscut.cod == CodRezultatEdu::ValidareEsuata &&
                             numarInvalid.cod == CodRezultatEdu::ValidareEsuata,
                         "validarea structurii cererilor a esuat");

                CerereActualizareLectieClient actualizare;
                actualizare.lectieId = lectieText;
                actualizare.cursId = curs1;
                actualizare.nume = "Text devenit video";
                actualizare.tip = TipLectieEdu::Video;
                actualizare.continut = "convertit.mp4";
                actualizare.dimensiuneOcteti = 4096;
                actualizare.durata = 90;
                actualizare.codec = "h265";
                client.actualizeazaLectie(actualizare);
                verifica(client.obtineLectie(lectieText)->tip == TipLectieEdu::Video,
                         "actualizarea cu schimbarea tipului a esuat");

                const auto enumInvalid = client.executaCerere(cerere(
                    TipCerereEdu::ActualizeazaLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(lectieVideo)},
                     {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Invalid"},
                     {static_cast<std::uint16_t>(CampEdu::TipLectie), "99"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"}}));
                const auto dateInvalide = client.executaCerere(cerere(
                    TipCerereEdu::ActualizeazaLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(lectieVideo)},
                     {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Invalid"},
                     {static_cast<std::uint16_t>(CampEdu::TipLectie), "2"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "-1"},
                     {static_cast<std::uint16_t>(CampEdu::Durata), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Codec), "h264"}}));
                verifica(enumInvalid.cod == CodRezultatEdu::ValidareEsuata &&
                             dateInvalide.cod == CodRezultatEdu::ValidareEsuata,
                         "enum-ul sau datele invalide au fost acceptate");
                verifica(client.obtineLectie(lectieVideo)->nume == "Video retea",
                         "actualizarea invalida a modificat lectia");

                const auto numarInainte = client.listeazaLectii(curs1).size();
                const auto conflict = client.executaCerere(cerere(
                    TipCerereEdu::CreeazaLectieVideo,
                    {{static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Video retea"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "duplicat"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Durata), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Codec), "x"}}));
                verifica(conflict.cod == CodRezultatEdu::Conflict &&
                             client.listeazaLectii(curs1).size() == numarInainte,
                         "conflictul a lasat o operatie partiala");
                verifica(client.trimiteCerere("PING") == "PONG",
                         "PING nu functioneaza dupa eroarea business");
            });

            ruleazaConexiune(server, [&](ClientEdu& client) {
                client.autentifica("profesor2.lectii@example.ro", "parola-2");
                const auto creareStraina = client.executaCerere(cerere(
                    TipCerereEdu::CreeazaLectieText,
                    {{static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs1)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Straina"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"},
                     {static_cast<std::uint16_t>(CampEdu::NumarCuvinte), "1"}}));
                const auto mutareStraina = client.executaCerere(cerere(
                    TipCerereEdu::ActualizeazaLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(lectieVideo)},
                     {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs2)},
                     {static_cast<std::uint16_t>(CampEdu::Nume), "Mutata"},
                     {static_cast<std::uint16_t>(CampEdu::TipLectie), "2"},
                     {static_cast<std::uint16_t>(CampEdu::Continut), "x"},
                     {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Durata), "1"},
                     {static_cast<std::uint16_t>(CampEdu::Codec), "x"}}));
                verifica(creareStraina.cod == CodRezultatEdu::AccesInterzis &&
                             mutareStraina.cod != CodRezultatEdu::Succes,
                         "profesorul neproprietar a administrat lectia");
                const auto stergereStraina = client.executaCerere(cerere(
                    TipCerereEdu::StergeLectie,
                    {{static_cast<std::uint16_t>(CampEdu::LectieId),
                      std::to_string(textInitial)}}));
                verifica(stergereStraina.cod == CodRezultatEdu::AccesInterzis &&
                             client.obtineLectie(textInitial).has_value(),
                         "profesorul neproprietar a sters lectia");
            });

            int lectieAdmin{};
            ruleazaConexiune(server, [&](ClientEdu& client) {
                client.autentifica("admin.lectii@example.ro", "parola-admin");
                lectieAdmin = client.creeazaLectieText(
                    curs2, "Lectie administrator", "continut", 8, 1);
                verifica(client.obtineLectie(lectieAdmin).has_value(),
                         "administratorul nu a creat lectia");
                client.stergeLectie(lectieAdmin);
                verifica(!client.obtineLectie(lectieAdmin).has_value(),
                         "administratorul nu a sters lectia");
            });

            ruleazaConexiune(server, [&](ClientEdu& client) {
                client.autentifica("profesor.lectii@example.ro", "parola-1");
                client.stergeLectie(lectieVideo);
                verifica(!client.obtineLectie(lectieVideo).has_value(),
                         "stergerea lectiei a esuat");
            });

            server.opresteNod();
        }

        conector.inchideConexiune();
        verifica(std::filesystem::remove(cale), "baza temporara nu a fost eliminata");
        verifica(SocketWindows::numarSocketuriActive() == socketuriInitiale,
                 "au ramas socket-uri active");
        verifica(InitializatorWinsock::numarInstanteActive() == winsockInitial,
                 "WSAStartup si WSACleanup nu sunt echilibrate");
        std::cout << "Testul de integrare a lectiilor prin retea a trecut.\n";
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
