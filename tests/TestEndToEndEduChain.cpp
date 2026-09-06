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
#include "ManagerSocket.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace {

void verifica(bool conditie, const char* mesaj) {
    if (!conditie) {
        throw std::runtime_error(mesaj);
    }
}

bool esteRespins(const std::function<void()>& operatie) {
    try {
        operatie();
    } catch (const ExceptieEdu&) {
        return true;
    }
    return false;
}

void adaugaProfesor(ConectorBazaDate& bazaDate, int utilizatorId) {
    bazaDate.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id, departament, data_angajarii) "
        "VALUES (?, ?, ?);",
        {std::to_string(utilizatorId), "E2E", "2026-07-19"});
    bazaDate.executaInterogareParametrizata(
        "INSERT INTO profesori (utilizator_id) VALUES (?);",
        {std::to_string(utilizatorId)});
}

void adaugaStudent(ConectorBazaDate& bazaDate, int utilizatorId) {
    bazaDate.executaInterogareParametrizata(
        "INSERT INTO studenti (utilizator_id) VALUES (?);",
        {std::to_string(utilizatorId)});
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

CerereEdu cerereFaraActor(TipCerereEdu tip, std::vector<CampProtocolEdu> campuri = {}) {
    CerereEdu cerere;
    cerere.tip = tip;
    cerere.campuri = std::move(campuri);
    return cerere;
}

} // namespace

int main() {
    const auto caleBazaDate = std::filesystem::temp_directory_path() /
        ("educhain_end_to_end_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".db");
    const int socketuriInitiale = SocketWindows::numarSocketuriActive();
    const int winsockInitial = InitializatorWinsock::numarInstanteActive();
    ConectorBazaDate bazaDate;

    try {
        std::filesystem::remove(caleBazaDate);
        bazaDate.deschideConexiune(caleBazaDate.string());

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

        const int profesorProprietar = utilizatori.adaugaUtilizator(
            "profesor.proprietar.e2e@example.ro", "profesor-e2e", "profesor");
        const int profesorStrain = utilizatori.adaugaUtilizator(
            "profesor.strain.e2e@example.ro", "profesor-strain", "profesor");
        const int studentInscris = utilizatori.adaugaUtilizator(
            "student.inscris.e2e@example.ro", "student-e2e", "student");
        const int studentNeinscris = utilizatori.adaugaUtilizator(
            "student.neinscris.e2e@example.ro", "student-neinscris", "student");
        adaugaProfesor(bazaDate, profesorProprietar);
        adaugaProfesor(bazaDate, profesorStrain);
        adaugaStudent(bazaDate, studentInscris);
        adaugaStudent(bazaDate, studentNeinscris);

        int cursId{};
        int lectieTextId{};
        int lectieVideoId{};
        int evaluareId{};
        int examenId{};
        int intrebareExamenId{};
        int intrebareCorectaId{};
        int intrebareGresitaId{};
        int incercareId{};

        {
            ServerEdu server(autentificare, cursuri, lectii, evaluari, inscrieri, 0);
            server.pornesteNod();

        ruleazaConexiune(server, [&](ClientEdu& client) {
            verifica(client.trimiteCerere("PING") == "PONG",
                     "PING-ul neautentificat a esuat");
            verifica(client.executaCerere(cerereFaraActor(TipCerereEdu::ListeazaCursuri)).cod ==
                         CodRezultatEdu::AccesInterzis,
                     "cererea neautentificata nu a fost respinsa");
            verifica(client.autentifica("profesor.proprietar.e2e@example.ro", "profesor-e2e").cod ==
                         CodRezultatEdu::Succes,
                     "autentificarea profesorului proprietar a esuat");
            verifica(client.trimiteCerere("PING") == "PONG",
                     "PING-ul autentificat a esuat");

            cursId = client.creeazaCurs("Curs E2E determinist");
            lectieTextId = client.creeazaLectieText(
                cursId, "Lectie text E2E", "Continut text", 13, 2);
            lectieVideoId = client.creeazaLectieVideo(
                cursId, "Lectie video E2E",
                "https://video.educhain.test/e2e.mp4", 1024, 60, "h264");
            evaluareId = client.creeazaChestionar(
                {cursId, "Evaluare E2E", 30, true, 2, std::nullopt});
            intrebareCorectaId = client.adaugaIntrebare(
                evaluareId, "Capitala Romaniei?", "Bucuresti", 1.0, 0);
            intrebareGresitaId = client.adaugaIntrebare(
                evaluareId, "Cat este 2 + 2?", "4", 2.0, 1);
            examenId = client.creeazaExamenFinal(
                {cursId, "Examen final E2E", 60, true, std::nullopt, 0.5});
            intrebareExamenId = client.adaugaIntrebare(
                examenId, "Protocolul EduChain foloseste TCP?", "da", 4.0, 0);
            client.inscrieStudentLaCurs(studentInscris, cursId);
            const auto studenti = client.listeazaStudentiCurs(cursId);
            verifica(studenti.size() == 1 && studenti.front().id == studentInscris,
                     "profesorul nu vede studentul inscris");
            verifica(esteRespins([&] {
                         client.inscrieStudentLaCurs(studentInscris, cursId);
                     }) && esteRespins([&] {
                         client.inscrieStudentLaCurs(999999, cursId);
                     }) && esteRespins([&] {
                         client.inscrieStudentLaCurs(profesorStrain, cursId);
                     }),
                     "inscrierea duplicata, utilizatorul inexistent sau rolul invalid nu au fost respinse");
            verifica(client.listeazaStudentiCurs(cursId).size() == 1,
                     "inscrierile respinse au modificat lista studentilor");
            client.deconecteaza();
            verifica(!client.esteAutentificat(), "sesiunea clientului nu a fost resetata");
        });
        ruleazaConexiune(server, [&](ClientEdu& client) {
            client.autentifica("student.inscris.e2e@example.ro", "student-e2e");
            const auto cursuriInscrise = client.listeazaCursuriInscrise();
            verifica(cursuriInscrise.size() == 1 && cursuriInscrise.front().id == cursId,
                     "studentul inscris nu vede exact cursul sau");
            verifica(client.listeazaCursuri().size() == 1 &&
                         client.obtineCurs(cursId).has_value(),
                     "detaliile cursului inscris nu sunt accesibile");
            const auto lectieVideo = client.obtineLectie(lectieVideoId);
            verifica(client.listeazaLectii(cursId).size() == 2 &&
                         client.obtineLectie(lectieTextId).has_value() &&
                         lectieVideo.has_value() &&
                         lectieVideo->continut ==
                             "https://video.educhain.test/e2e.mp4",
                     "lectiile cursului inscris nu sunt accesibile");
            verifica(client.listeazaEvaluari(cursId).size() == 2,
                     "evaluarile cursului inscris nu sunt accesibile");
            const auto intrebari = client.listeazaIntrebari(evaluareId);
            verifica(intrebari.size() == 2 &&
                         intrebari[0].enunt == "Capitala Romaniei?" &&
                         intrebari[1].enunt == "Cat este 2 + 2?",
                     "intrebarile publice sunt invalide");
            const auto intrebariExamen = client.listeazaIntrebari(examenId);
            verifica(intrebariExamen.size() == 1 &&
                         intrebariExamen.front().id == intrebareExamenId,
                     "intrebarea examenului final nu este accesibila");

            const auto rezultateInitiale = client.listeazaRezultateleMele();
            verifica(rezultateInitiale.size() == 2 &&
                         !rezultateInitiale[0].sustinuta &&
                         !rezultateInitiale[1].sustinuta,
                     "statusul initial al evaluarilor este invalid");

            incercareId = client.pornesteIncercare(evaluareId);
            verifica(esteRespins([&] { client.pornesteIncercare(evaluareId); }),
                     "doua cereri consecutive au creat incercari duplicate");
            client.salveazaRaspuns(incercareId, intrebareCorectaId, "  bUcUrEsTi\t");
            client.salveazaRaspuns(incercareId, intrebareGresitaId, "3");
            const auto rezultat = client.finalizeazaIncercare(incercareId);
            verifica(std::fabs(rezultat.scorBrut - 1.0) < 0.0001 &&
                         std::fabs(rezultat.notaFinala - 3.33) < 0.0001,
                     "scorul sau nota calculata server-side sunt incorecte");
            verifica(esteRespins([&] {
                         client.salveazaRaspuns(incercareId, intrebareGresitaId, "4");
                     }) && esteRespins([&] { client.finalizeazaIncercare(incercareId); }) &&
                         esteRespins([&] { client.pornesteIncercare(evaluareId); }),
                     "incercarea finalizata a putut fi modificata, finalizata sau pornita din nou");

            const int incercareExamen = client.pornesteIncercare(examenId);
            client.salveazaRaspuns(incercareExamen, intrebareExamenId, "DA");
            const auto rezultatExamen = client.finalizeazaIncercare(incercareExamen);
            verifica(std::fabs(rezultatExamen.scorBrut - 4.0) < 0.0001 &&
                         std::fabs(rezultatExamen.notaFinala - 10.0) < 0.0001,
                     "rezultatul examenului final este incorect");

            const auto rezultateFinale = client.listeazaRezultateleMele();
            verifica(rezultateFinale.size() == 2 &&
                         rezultateFinale[0].sustinuta && rezultateFinale[0].finalizataLa &&
                         rezultateFinale[1].sustinuta && rezultateFinale[1].finalizataLa,
                     "studentul nu isi poate lista rezultatele finalizate");
            CerereEdu injectieStudent;
            injectieStudent.tip = TipCerereEdu::ListeazaRezultateleMele;
            injectieStudent.campuri = {
                {static_cast<std::uint16_t>(CampEdu::StudentId),
                 std::to_string(studentNeinscris)}};
            verifica(client.executaCerere(injectieStudent).cod ==
                         CodRezultatEdu::ValidareEsuata,
                     "studentul poate injecta ID-ul altui student in lista de rezultate");
            client.deconecteaza();
        });
        ruleazaConexiune(server, [&](ClientEdu& client) {
            client.autentifica("profesor.proprietar.e2e@example.ro", "profesor-e2e");
            const auto rezultateChestionar =
                client.listeazaRezultateleEvaluarii(evaluareId);
            const auto rezultateExamen = client.listeazaRezultateleEvaluarii(examenId);
            verifica(rezultateChestionar.size() == 1 &&
                         rezultateChestionar.front().studentId == studentInscris &&
                         rezultateChestionar.front().sustinuta &&
                         std::fabs(rezultateChestionar.front().punctajMaxim - 3.0) < 0.0001 &&
                         rezultateExamen.size() == 1 && rezultateExamen.front().sustinuta &&
                         std::fabs(rezultateExamen.front().notaFinala - 10.0) < 0.0001,
                     "profesorul proprietar nu vede rezultatele corecte ale studentului");
        });
        ruleazaConexiune(server, [&](ClientEdu& client) {
            client.autentifica("student.neinscris.e2e@example.ro", "student-neinscris");
            verifica(client.listeazaCursuriInscrise().empty() && client.listeazaCursuri().empty(),
                     "studentul neinscris vede cursuri");
            verifica(esteRespins([&] { client.obtineCurs(cursId); }) &&
                         esteRespins([&] { client.listeazaLectii(cursId); }) &&
                         esteRespins([&] { client.listeazaEvaluari(cursId); }) &&
                         esteRespins([&] { client.listeazaIntrebari(evaluareId); }) &&
                         esteRespins([&] { client.listeazaRezultateleEvaluarii(evaluareId); }) &&
                         esteRespins([&] { client.pornesteIncercare(evaluareId); }),
                     "studentul neinscris a accesat continutul cursului");
            verifica(esteRespins([&] { client.creeazaCurs("Interzis"); }) &&
                         esteRespins([&] {
                             client.creeazaLectieText(cursId, "Interzisa", "x", 1, 1);
                         }) && esteRespins([&] {
                             client.creeazaChestionar({cursId, "Interzisa", 1, true, 1, std::nullopt});
                         }) && esteRespins([&] {
                             client.adaugaIntrebare(evaluareId, "Interzisa", "x", 1.0, 2);
                         }),
                     "studentul a primit drepturi de administrare");
            client.deconecteaza();
        });
        ruleazaConexiune(server, [&](ClientEdu& client) {
            client.autentifica("profesor.strain.e2e@example.ro", "profesor-strain");
            verifica(client.listeazaCursuri().empty() &&
                         esteRespins([&] { client.obtineCurs(cursId); }) &&
                         esteRespins([&] { client.listeazaStudentiCurs(cursId); }) &&
                         esteRespins([&] { client.listeazaLectii(cursId); }) &&
                         esteRespins([&] { client.obtineLectie(lectieTextId); }) &&
                         esteRespins([&] { client.listeazaEvaluari(cursId); }) &&
                         esteRespins([&] { client.obtineEvaluare(evaluareId); }) &&
                         esteRespins([&] { client.listeazaIntrebari(evaluareId); }) &&
                         esteRespins([&] { client.listeazaRezultateleEvaluarii(evaluareId); }) &&
                         esteRespins([&] { client.actualizeazaCurs(cursId, "Curs furat"); }) &&
                         esteRespins([&] { client.stergeCurs(cursId); }) &&
                         esteRespins([&] {
                             client.creeazaLectieText(cursId, "Lectie straina", "x", 1, 1);
                         }) && esteRespins([&] {
                             client.stergeEvaluare(evaluareId);
                         }) && esteRespins([&] {
                             client.adaugaIntrebare(evaluareId, "Intrebare straina", "x", 1.0, 2);
                         }) && esteRespins([&] {
                             client.adaugaIntrebare(examenId, "Intrebare examen straina", "x", 1.0, 2);
                         }) && esteRespins([&] {
                             client.inscrieStudentLaCurs(studentNeinscris, cursId);
                         }),
                     "profesorul strain a citit sau administrat cursul");
            verifica(client.trimiteCerere("PING") == "PONG",
                     "conexiunea nu a ramas functionala dupa eroare business");
        });
        server.opresteNod();
    } // ServerEdu este distrus inaintea verificarilor de resurse Winsock.

        verifica(cursuriRepository.cautaDupaId(cursId).has_value(),
                 "cursul nu a fost persistat");
        verifica(lectiiRepository.listeazaDupaCurs(cursId).size() == 2,
                 "lectiile nu au fost persistate");
        verifica(evaluariRepository.cautaDupaId(evaluareId).has_value() &&
                     evaluariRepository.listeazaIntrebari(evaluareId).size() == 2 &&
                     evaluariRepository.cautaDupaId(examenId).has_value() &&
                     evaluariRepository.listeazaIntrebari(examenId).size() == 1,
                 "evaluarea sau intrebarile nu au fost persistate");
        verifica(inscrieriRepository.esteInscris(studentInscris, cursId) &&
                     !inscrieriRepository.esteInscris(studentNeinscris, cursId),
                 "inscrierile sau refuzul profesorului strain nu au fost persistate corect");
        const auto incercare = evaluariRepository.cautaIncercareDupaId(incercareId);
        verifica(incercare.has_value() && incercare->finalizataLa.has_value() &&
                     std::fabs(incercare->scorBrut - 1.0) < 0.0001 &&
                     std::fabs(incercare->notaFinala - 3.33) < 0.0001 &&
                     evaluariRepository.listeazaRaspunsuri(incercareId).size() == 2,
                 "incercarea, raspunsurile sau rezultatul final nu au fost persistate");

        bazaDate.inchideConexiune();
        verifica(std::filesystem::remove(caleBazaDate),
                 "baza temporara nu a fost eliminata");
        verifica(SocketWindows::numarSocketuriActive() == socketuriInitiale,
                 "au ramas socket-uri active");
        verifica(InitializatorWinsock::numarInstanteActive() == winsockInitial,
                 "WSAStartup si WSACleanup nu sunt echilibrate");
        std::cout << "Test End-to-End EduChain: SUCCES\n";
        return 0;
    } catch (const std::exception& exceptie) {
        if (bazaDate.esteConectat()) {
            try {
                bazaDate.inchideConexiune();
            } catch (...) {
            }
        }
        std::error_code eroare;
        std::filesystem::remove(caleBazaDate, eroare);
        std::cerr << exceptie.what() << '\n';
        return 1;
    }
}
