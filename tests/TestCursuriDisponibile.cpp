#include "AutentificareService.h"
#include "ClientEdu.h"
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

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
void verifica(bool conditie, const char* mesaj) {
    if (!conditie) throw std::runtime_error(mesaj);
}
bool esueaza(const std::function<void()>& operatie) {
    try { operatie(); } catch (const ExceptieEdu&) { return true; }
    return false;
}
void adaugaStudent(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO studenti(utilizator_id) VALUES (?);", {std::to_string(id)});
}
void adaugaProfesor(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO personal(utilizator_id,departament,data_angajarii) VALUES (?,?,?);",
        {std::to_string(id), "IT", "2026-07-20"});
    db.executaInterogareParametrizata(
        "INSERT INTO profesori(utilizator_id) VALUES (?);", {std::to_string(id)});
}
template<class Operatie>
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
        ("educhain_cursuri_disponibile_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".db");
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

        const int profesor = utilizatori.adaugaUtilizator(
            "prof@x.ro", "p", "profesor", "Ionescu", "Mihai");
        const int student1 = utilizatori.adaugaUtilizator(
            "s1@x.ro", "s", "student", "Popescu", "Ion");
        const int student2 = utilizatori.adaugaUtilizator(
            "s2@x.ro", "s", "student", "Georgescu", "Ana");
        adaugaProfesor(db, profesor); adaugaStudent(db, student1);
        adaugaStudent(db, student2);
        const int curs1 = cursuri.creeazaCurs({profesor, profesor, "Curs 1", std::nullopt});
        const int curs2 = cursuri.creeazaCurs({profesor, profesor, "Curs 2", curs1});
        const int evaluare = evaluari.creeazaEvaluare(
            {profesor, curs1, "Evaluare disponibila", "chestionar", 10, true, 0,
             std::nullopt});
        evaluari.adaugaIntrebare(
            {profesor, evaluare, "Intrebare", "raspuns", 1.0, 0});

        ServerEdu server(autentificare, cursuri, lectii, evaluari, inscrieri, 0);
        server.pornesteNod();
        conexiune(server, [&](ClientEdu& client) {
            CerereEdu cerere; cerere.tip = TipCerereEdu::ListeazaCursuriDisponibile;
            verifica(client.executaCerere(cerere).cod == CodRezultatEdu::AccesInterzis,
                     "cererea neautentificata a fost acceptata");
        });
        conexiune(server, [&](ClientEdu& client) {
            client.autentifica("s1@x.ro", "s");
            verifica(client.listeazaCursuriDisponibile().size() == 2,
                     "studentul fara inscrieri nu vede toate cursurile");
            verifica(client.listeazaCursuri().empty() &&
                     client.listeazaCursuriInscrise().empty(),
                     "semantica metodelor vechi s-a schimbat");
            client.inscrieLaCurs(curs1);
            const auto disponibile = client.listeazaCursuriDisponibile();
            verifica(disponibile.size() == 1 && disponibile.front().id == curs2,
                     "cursul inscris a ramas disponibil");
            verifica(client.listeazaCursuriInscrise().size() == 1,
                     "inscrierea nu a actualizat lista veche");
            const auto rezultate = client.listeazaRezultateleMele();
            verifica(rezultate.size() == 1 &&
                         rezultate.front().evaluareId == evaluare &&
                         !rezultate.front().sustinuta &&
                         rezultate.front().punctajMaxim == 1.0,
                     "evaluarea nesustinuta nu apare corect in rezultatele studentului");
            verifica(esueaza([&] { client.inscrieLaCurs(curs1); }),
                     "inscrierea duplicata nu a fost respinsa");
            verifica(client.trimiteCerere("PING") == "PONG",
                     "conexiunea nu a ramas activa dupa conflict");
            client.inscrieLaCurs(curs2);
            verifica(client.listeazaCursuriDisponibile().empty(),
                     "studentul inscris la toate cursurile nu primeste lista goala");
        });
        conexiune(server, [&](ClientEdu& client) {
            client.autentifica("s2@x.ro", "s");
            verifica(client.listeazaCursuriDisponibile().size() == 2,
                     "listele studentilor nu sunt independente");
        });
        conexiune(server, [&](ClientEdu& client) {
            client.autentifica("prof@x.ro", "p");
            verifica(esueaza([&] { client.listeazaCursuriDisponibile(); }),
                     "profesorul a primit cursuri disponibile");
            const auto rezultate = client.listeazaRezultateleEvaluarii(evaluare);
            verifica(rezultate.size() == 1 &&
                         rezultate.front().studentId == student1 &&
                         rezultate.front().numeStudent == "Popescu" &&
                         rezultate.front().prenumeStudent == "Ion" &&
                         !rezultate.front().sustinuta,
                     "profesorul nu vede statusul studentului inscris");
            verifica(client.trimiteCerere("PING") == "PONG", "conexiune profesor inchisa");
        });
        server.opresteNod();
        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cout << "Test cursuri disponibile: SUCCES\n";
        return 0;
    } catch (const std::exception& eroare) {
        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cerr << eroare.what() << '\n';
        return 1;
    }
}
