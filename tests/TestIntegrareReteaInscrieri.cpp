#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "EvaluareRepository.h"
#include "EvaluareService.h"
#include "InscriereRepository.h"
#include "InscriereService.h"
#include "LectieRepository.h"
#include "LectieService.h"
#include "ManagerSocket.h"
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

void adaugaProfesor(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO personal(utilizator_id,departament,data_angajarii) VALUES (?,?,?);",
        {std::to_string(id), "IT", "2026-07-19"});
    db.executaInterogareParametrizata(
        "INSERT INTO profesori(utilizator_id) VALUES (?);", {std::to_string(id)});
}

void adaugaStudent(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO studenti(utilizator_id) VALUES (?);", {std::to_string(id)});
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

CerereEdu cerereInscriere(int studentId, int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::InscrieStudentLaCurs;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::StudentId), std::to_string(studentId)},
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(cursId)}};
    return cerere;
}
}

int main() {
    const auto cale = std::filesystem::temp_directory_path() /
        ("educhain_retea_inscrieri_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".db");
    const int socketuri = SocketWindows::numarSocketuriActive();
    const int winsock = InitializatorWinsock::numarInstanteActive();
    ConectorBazaDate db;
    try {
        db.deschideConexiune(cale.string());
        UtilizatorRepository utilizatori(db);
        CursRepository cursuriRepo(db);
        LectieRepository lectiiRepo(db);
        EvaluareRepository evaluariRepo(db);
        InscriereRepository inscrieriRepo(db);
        InscriereService inscrieri(inscrieriRepo, cursuriRepo, utilizatori);
        CursService cursuri(cursuriRepo, utilizatori, inscrieri);
        LectieService lectii(lectiiRepo, cursuriRepo, utilizatori, inscrieri);
        EvaluareService evaluari(evaluariRepo, cursuriRepo, utilizatori, inscrieri);
        AutentificareService autentificare(utilizatori);

        const int profesor1 = utilizatori.adaugaUtilizator(
            "p1@x.ro", "parola1", "profesor", "Ionescu", "Mihai");
        const int profesor2 = utilizatori.adaugaUtilizator(
            "p2@x.ro", "parola2", "profesor", "Marin", "Elena");
        const int student1 = utilizatori.adaugaUtilizator(
            "s1@x.ro", "parola3", "student", "Popescu", "Ion");
        const int student2 = utilizatori.adaugaUtilizator(
            "s2@x.ro", "parola4", "student", "Georgescu", "Ana");
        adaugaProfesor(db, profesor1); adaugaProfesor(db, profesor2);
        adaugaStudent(db, student1); adaugaStudent(db, student2);
        const int curs1 = cursuri.creeazaCurs(
            {profesor1, profesor1, "Curs 1", std::nullopt});
        const int curs2 = cursuri.creeazaCurs(
            {profesor2, profesor2, "Curs 2", std::nullopt});

        {
            ServerEdu server(autentificare, cursuri, lectii, evaluari, inscrieri, 0);
            server.pornesteNod();
            conexiune(server, [&](ClientEdu& client) {
                CerereEdu cerere;
                cerere.tip = TipCerereEdu::ListeazaStudentiCurs;
                cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                                   std::to_string(curs1)}};
                verifica(client.executaCerere(cerere).cod == CodRezultatEdu::AccesInterzis,
                         "listarea fara autentificare a fost acceptata");
            });
            conexiune(server, [&](ClientEdu& client) {
                const auto login = client.autentifica("p1@x.ro", "parola1");
                verifica(login.cod == CodRezultatEdu::Succes,
                         "login profesor a esuat");
                verifica(!ProtocolEdu::cautaCamp(login.campuri, CampEdu::Parola),
                         "hash-ul parolei a fost trimis prin protocol");
                const auto eligibiliInitial = client.listeazaStudentiEligibili(curs1);
                verifica(eligibiliInitial.size() == 2,
                         "lista initiala de studenti eligibili este incorecta");
                verifica(client.executaCerere(cerereInscriere(student1, curs1)).cod ==
                             CodRezultatEdu::Succes,
                         "studentul existent nu a fost inscris");
                const auto lista = client.listeazaStudentiCurs(curs1);
                verifica(lista.size() == 1 && lista.front().id == student1 &&
                             lista.front().nume == "Popescu" &&
                             lista.front().prenume == "Ion",
                         "StudentPublicEdu nu contine numele si prenumele");
                const auto eligibiliDupaInscriere = client.listeazaStudentiEligibili(curs1);
                verifica(eligibiliDupaInscriere.size() == 1 &&
                             eligibiliDupaInscriere.front().id == student2,
                         "studentul deja inscris nu a fost exclus dintre eligibili");

                const auto duplicat = client.executaCerere(cerereInscriere(student1, curs1));
                verifica(duplicat.cod == CodRezultatEdu::Conflict &&
                             duplicat.mesajPublic ==
                                 "Studentul este deja inscris la acest curs.",
                         "duplicatul nu este refuzat explicit");
                const auto inexistent = client.executaCerere(cerereInscriere(999999, curs1));
                verifica(inexistent.cod == CodRezultatEdu::ResursaInexistenta &&
                             inexistent.mesajPublic == "Studentul nu exista.",
                         "studentul inexistent nu este refuzat explicit");
                const auto rolInvalid = client.executaCerere(
                    cerereInscriere(profesor2, curs1));
                verifica(rolInvalid.cod == CodRezultatEdu::ValidareEsuata &&
                             rolInvalid.mesajPublic ==
                                 "Utilizatorul selectat nu este student.",
                         "profesorul poate fi inscris ca student");
                CerereEdu lipsaCamp;
                lipsaCamp.tip = TipCerereEdu::InscrieStudentLaCurs;
                lipsaCamp.campuri = {{static_cast<std::uint16_t>(CampEdu::StudentId),
                                      std::to_string(student2)}};
                verifica(client.executaCerere(lipsaCamp).cod == CodRezultatEdu::ValidareEsuata,
                         "cererea cu camp lipsa a fost acceptata");
            });
            conexiune(server, [&](ClientEdu& client) {
                client.autentifica("p2@x.ro", "parola2");
                verifica(client.executaCerere(cerereInscriere(student2, curs1)).cod ==
                             CodRezultatEdu::AccesInterzis,
                         "profesorul strain a inscris student in cursul altuia");
                verifica(client.executaCerere(cerereInscriere(student2, curs2)).cod ==
                             CodRezultatEdu::Succes,
                         "proprietarul nu poate inscrie studentul");
            });
            server.opresteNod();
        }

        db.inchideConexiune();
        std::filesystem::remove(cale);
        verifica(SocketWindows::numarSocketuriActive() == socketuri,
                 "au ramas socketuri active");
        verifica(InitializatorWinsock::numarInstanteActive() == winsock,
                 "au ramas instante Winsock active");
        std::cout << "Test retea inscrieri: SUCCES\n";
        return 0;
    } catch (const std::exception& eroare) {
        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cerr << eroare.what() << '\n';
        return 1;
    }
}
