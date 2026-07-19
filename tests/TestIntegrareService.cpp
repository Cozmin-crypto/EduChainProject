#include "AutentificareService.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "EvaluareRepository.h"
#include "EvaluareService.h"
#include "ExceptieEdu.h"
#include "LectieRepository.h"
#include "LectieService.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

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
                            .time_since_epoch()
                            .count();
    return std::filesystem::temp_directory_path() /
           ("educhain_service_" + std::to_string(marcaj) + ".db");
}

void adaugaProfesor(ConectorBazaDate& conector, int id) {
    // Nu exista servicii/repository-uri publice pentru specializarile personal/profesor.
    conector.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id, departament, data_angajarii) "
        "VALUES (?, ?, ?);", {std::to_string(id), "Informatica", "2026-07-18"});
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
        "VALUES (?, ?, ?);", {std::to_string(id), "Administratie", "2026-07-18"});
    conector.executaInterogareParametrizata(
        "INSERT INTO administratori (utilizator_id, nivel_acces) VALUES (?, ?);",
        {std::to_string(id), "10"});
}
}

int main() {
    const auto cale = caleTemporara();
    ConectorBazaDate conector;
    try {
        std::filesystem::remove(cale);
        conector.deschideConexiune(cale.string());

        UtilizatorRepository utilizatori(conector);
        CursRepository cursuriRepository(conector);
        LectieRepository lectiiRepository(conector);
        EvaluareRepository evaluariRepository(conector);

        AutentificareService autentificare(utilizatori);
        CursService cursuri(cursuriRepository, utilizatori);
        LectieService lectii(lectiiRepository, cursuriRepository, utilizatori);
        EvaluareService evaluari(evaluariRepository, cursuriRepository, utilizatori);

        const int profesor1 = utilizatori.adaugaUtilizator(
            "profesor1@example.ro", "parola-1", "profesor");
        const int profesor2 = utilizatori.adaugaUtilizator(
            "profesor2@example.ro", "parola-2", "profesor");
        const int student = utilizatori.adaugaUtilizator(
            "student@example.ro", "parola-student", "student");
        const int administrator = utilizatori.adaugaUtilizator(
            "admin@example.ro", "parola-admin", "administrator");
        adaugaProfesor(conector, profesor1);
        adaugaProfesor(conector, profesor2);
        adaugaStudent(conector, student);
        adaugaAdministrator(conector, administrator);

        const auto autentificareReusita = autentificare.autentifica(
            "profesor1@example.ro", "parola-1");
        verifica(autentificareReusita.succes &&
                     autentificareReusita.utilizatorId == profesor1 &&
                     autentificareReusita.rol == "profesor",
                 "autentificarea valida a esuat");
        const auto emailInexistent = autentificare.autentifica(
            "absent@example.ro", "parola");
        const auto parolaGresita = autentificare.autentifica(
            "profesor1@example.ro", "gresita");
        verifica(emailInexistent.stare == StareAutentificare::EmailInexistent,
                 "emailul inexistent nu este diferentiat intern");
        verifica(parolaGresita.stare == StareAutentificare::ParolaIncorecta,
                 "parola gresita nu este diferentiata intern");
        verifica(emailInexistent.mesajPublic == parolaGresita.mesajPublic,
                 "mesajul public expune existenta contului");
        verifica(aruncaExceptieEdu([&] { autentificare.autentifica("", "parola"); }),
                 "datele de autentificare invalide nu sunt respinse");

        const int cursProfesor1 = cursuri.creeazaCurs(
            {profesor1, profesor1, "Curs profesor 1", std::nullopt});
        verifica(cursuri.actualizeazaCurs(
                     {profesor1, cursProfesor1, "Curs profesor 1 actualizat", std::nullopt}),
                 "profesorul nu si-a putut actualiza cursul");
        const int cursProfesor2 = cursuri.creeazaCurs(
            {profesor2, profesor2, "Curs profesor 2", std::nullopt});

        const std::string numeInitialProfesor2 =
            cursuri.obtineCurs(cursProfesor2)->nume;
        verifica(aruncaExceptieEdu([&] {
                     cursuri.actualizeazaCurs(
                         {profesor1, cursProfesor2, "Modificare interzisa", std::nullopt});
                 }),
                 "profesorul a modificat cursul altui profesor");
        verifica(cursuri.obtineCurs(cursProfesor2)->nume == numeInitialProfesor2,
                 "refuzul autorizarii a modificat cursul");
        verifica(aruncaExceptieEdu([&] {
                     cursuri.creeazaCurs(
                         {student, profesor1, "Curs student", std::nullopt});
                 }),
                 "studentul a creat un curs");
        verifica(aruncaExceptieEdu([&] {
                     cursuri.actualizeazaCurs(
                         {student, cursProfesor1, "Curs student", std::nullopt});
                 }),
                 "studentul a modificat un curs");

        const int cursCreatDeAdmin = cursuri.creeazaCurs(
            {administrator, profesor2, "Curs creat de administrator", std::nullopt});
        verifica(cursuri.obtineCurs(cursCreatDeAdmin)->proprietarId == profesor2,
                 "administratorul a fost setat incorect drept proprietar");
        verifica(cursuri.actualizeazaCurs(
                     {administrator, cursProfesor1, "Curs administrat", std::nullopt}),
                 "administratorul nu a putut modifica un curs");
        verifica(cursuri.stergeCurs(administrator, cursCreatDeAdmin),
                 "administratorul nu a putut sterge un curs");

        CerereSalvareLectie cerereLectie;
        cerereLectie.actorId = profesor1;
        cerereLectie.cursId = cursProfesor1;
        cerereLectie.nume = "Lectie text";
        cerereLectie.tip = "text";
        cerereLectie.continut = "Text cu ' OR 1=1 --";
        cerereLectie.dimensiuneOcteti = 20;
        cerereLectie.numarCuvinte = 5;
        const int lectieId = lectii.creeazaLectie(cerereLectie);
        verifica(lectii.listeazaDupaCurs(cursProfesor1).size() == 1,
                 "lectia profesorului nu este listata");
        verifica(aruncaExceptieEdu([&] {
                     auto interzisa = cerereLectie;
                     interzisa.actorId = profesor2;
                     interzisa.nume = "Lectie interzisa";
                     lectii.creeazaLectie(interzisa);
                 }),
                 "alt profesor a administrat lectiile cursului");
        verifica(lectii.listeazaDupaCurs(cursProfesor1).size() == 1,
                 "operatia refuzata a creat partial o lectie");

        CerereActualizareLectie actualizareLectie;
        actualizareLectie.actorId = profesor1;
        actualizareLectie.cursId = cursProfesor1;
        actualizareLectie.lectieId = lectieId;
        actualizareLectie.nume = "Lectie video";
        actualizareLectie.tip = "video";
        actualizareLectie.continut = "video.mp4";
        actualizareLectie.dimensiuneOcteti = 2048;
        actualizareLectie.durata = 120;
        actualizareLectie.codec = "h264";
        verifica(lectii.actualizeazaLectie(actualizareLectie),
                 "profesorul nu si-a actualizat lectia");

        CerereSalvareEvaluare cerereEvaluare;
        cerereEvaluare.actorId = profesor1;
        cerereEvaluare.cursId = cursProfesor1;
        cerereEvaluare.nume = "Chestionar servicii";
        cerereEvaluare.tip = "chestionar";
        cerereEvaluare.limitaTimp = 30;
        cerereEvaluare.esteObligatorie = true;
        cerereEvaluare.numarIntrebari = 0;
        const int chestionarId = evaluari.creeazaEvaluare(cerereEvaluare);
        const int intrebare1 = evaluari.adaugaIntrebare(
            {profesor1, chestionarId, "Intrebare initiala", "da", 5.0, 0});
        const int intrebare2 = evaluari.adaugaIntrebare(
            {profesor1, chestionarId, "Intrebare secunda", "nu", 5.0, 1});

        verifica(evaluariRepository.actualizeazaIntrebare(
                     intrebare1, "Text actualizat '); DROP TABLE evaluari; --", "da", 7.5, 2),
                 "actualizarea intrebarii a esuat");
        const auto intrebariActualizate = evaluari.listeazaIntrebari(chestionarId);
        verifica(intrebariActualizate.back().enunt ==
                     "Text actualizat '); DROP TABLE evaluari; --",
                 "textul SQL al intrebarii nu a fost tratat literal");
        verifica(!evaluariRepository.actualizeazaIntrebare(
                     999999, "Inexistenta", "da", 1.0, 3),
                 "ID-ul inexistent nu a returnat false");
        verifica(aruncaExceptieEdu([&] {
                     evaluariRepository.actualizeazaIntrebare(intrebare1, "", "da", 1.0, 3);
                 }), "enuntul invalid nu a fost respins");
        verifica(aruncaExceptieEdu([&] {
                     evaluariRepository.actualizeazaIntrebare(intrebare1, "Valid", "da", -1.0, 3);
                 }), "punctajul invalid nu a fost respins");
        verifica(aruncaExceptieEdu([&] {
                     evaluariRepository.actualizeazaIntrebare(intrebare1, "Valid", "da", 1.0, -1);
                 }), "ordinea invalida nu a fost respinsa");
        verifica(aruncaExceptieEdu([&] {
                     evaluariRepository.actualizeazaIntrebare(intrebare1, "Conflict", "da", 1.0, 1);
                 }), "conflictul UNIQUE nu a fost propagat");
        verifica(evaluariRepository.listeazaIntrebari(chestionarId).size() == 2,
                 "conflictul UNIQUE a alterat intrebarile");

        CerereActualizareIntrebare cerereActualizareIntrebare;
        cerereActualizareIntrebare.actorId = profesor1;
        cerereActualizareIntrebare.chestionarId = chestionarId;
        cerereActualizareIntrebare.intrebareId = intrebare1;
        cerereActualizareIntrebare.enunt = "Intrebare actualizata prin serviciu";
        cerereActualizareIntrebare.raspunsCorect = "Raspuns student";
        cerereActualizareIntrebare.punctajMaxim = 8.0;
        cerereActualizareIntrebare.ordine = 2;
        verifica(evaluari.actualizeazaIntrebare(cerereActualizareIntrebare),
                 "serviciul nu a actualizat intrebarea");
        verifica(aruncaExceptieEdu([&] {
                     auto interzisa = cerereActualizareIntrebare;
                     interzisa.actorId = profesor2;
                     evaluari.actualizeazaIntrebare(interzisa);
                 }), "alt profesor a administrat evaluarea");

        const int incercareId = evaluari.pornesteIncercare(student, chestionarId);
        verifica(aruncaExceptieEdu([&] {
                     evaluari.pornesteIncercare(profesor1, chestionarId);
                 }), "profesorul a pornit o incercare de student");

        auto altaEvaluare = cerereEvaluare;
        altaEvaluare.nume = "Alt chestionar";
        const int altChestionarId = evaluari.creeazaEvaluare(altaEvaluare);
        const int intrebareStraina = evaluari.adaugaIntrebare(
            {profesor1, altChestionarId, "Intrebare straina", "strain", 2.0, 0});
        verifica(aruncaExceptieEdu([&] {
                     evaluari.salveazaRaspuns(
                         {student, incercareId, intrebareStraina, "Raspuns invalid"});
                 }), "studentul a raspuns unei intrebari din alta evaluare");
        verifica(evaluariRepository.listeazaRaspunsuri(incercareId).empty(),
                 "raspunsul invalid a fost persistat partial");
        verifica(evaluari.stergeEvaluare(profesor1, altChestionarId),
                 "chestionarul auxiliar nu a fost sters");

        evaluari.salveazaRaspuns(
            {student, incercareId, intrebare1, "Raspuns student"});
        const auto finalizata = evaluari.finalizeazaIncercare(student, incercareId);
        verifica(finalizata.finalizataLa.has_value(), "studentul nu si-a finalizat incercarea");
        const auto rezultat = evaluari.obtineIncercare(student, incercareId);
        verifica(rezultat->finalizataLa.has_value() && rezultat->notaFinala == 6.15,
                 "rezultatul incercarii este incorect");
        verifica(evaluari.listeazaDupaCurs(cursProfesor1).size() == 1,
                 "evaluarea nu este listata");

        // Evita avertismentul pentru variabila folosita doar la conflictul UNIQUE.
        verifica(intrebare2 > 0, "a doua intrebare nu a fost creata");

        conector.inchideConexiune();
        verifica(std::filesystem::remove(cale),
                 "baza temporara a serviciilor nu a fost eliminata");
        std::cout << "Testul de integrare Service Layer a trecut.\n";
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
