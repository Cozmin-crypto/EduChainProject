#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "ExceptieEdu.h"
#include "InscriereRepository.h"
#include "InscriereService.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
void verifica(bool conditie, const char* mesaj) {
    if (!conditie) throw std::runtime_error(mesaj);
}

std::string mesajExceptie(const std::function<void()>& operatie) {
    try { operatie(); } catch (const ExceptieEdu& eroare) { return eroare.what(); }
    return {};
}

void adaugaProfesor(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id,departament,data_angajarii) "
        "VALUES (?,?,?);", {std::to_string(id), "IT", "2026-07-19"});
    db.executaInterogareParametrizata(
        "INSERT INTO profesori(utilizator_id) VALUES (?);", {std::to_string(id)});
}

void adaugaStudent(ConectorBazaDate& db, int id) {
    db.executaInterogareParametrizata(
        "INSERT INTO studenti(utilizator_id) VALUES (?);", {std::to_string(id)});
}
}

int main() {
    const auto cale = std::filesystem::temp_directory_path() /
        ("educhain_inscrieri_" + std::to_string(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()) + ".db");
    ConectorBazaDate db;
    try {
        db.deschideConexiune(cale.string());
        UtilizatorRepository utilizatori(db);
        CursRepository cursuri(db);
        InscriereRepository repository(db);
        InscriereService serviciu(repository, cursuri, utilizatori);

        const int profesor1 = utilizatori.adaugaUtilizator(
            "p1@x.ro", "p", "profesor", "Ionescu", "Mihai");
        const int profesor2 = utilizatori.adaugaUtilizator(
            "p2@x.ro", "p", "profesor", "Marin", "Elena");
        const int student1 = utilizatori.adaugaUtilizator(
            "s1@x.ro", "s", "student", "Popescu", "Ion");
        const int student2 = utilizatori.adaugaUtilizator(
            "s2@x.ro", "s", "student", "Georgescu", "Ana");
        adaugaProfesor(db, profesor1); adaugaProfesor(db, profesor2);
        adaugaStudent(db, student1); adaugaStudent(db, student2);

        const int curs1 = cursuri.adaugaCurs("Curs 1", std::nullopt, profesor1);
        const int curs2 = cursuri.adaugaCurs("Curs 2", std::nullopt, profesor2);

        serviciu.inscrieStudent(profesor1, student1, curs1);
        verifica(repository.esteInscris(student1, curs1),
                 "studentul existent nu a fost inscris");
        verifica(mesajExceptie([&] { serviciu.inscrieStudent(profesor1, student1, curs1); }) ==
                     "Studentul este deja inscris la acest curs.",
                 "duplicatul nu are mesajul public asteptat");
        verifica(mesajExceptie([&] { serviciu.inscrieStudent(profesor1, 99999, curs1); }) ==
                     "Studentul nu exista.",
                 "studentul inexistent nu este diferentiat");
        verifica(mesajExceptie([&] { serviciu.inscrieStudent(profesor1, profesor2, curs1); }) ==
                     "Utilizatorul selectat nu este student.",
                 "rolul profesor nu este respins");
        verifica(!mesajExceptie([&] { serviciu.inscrieStudent(profesor2, student2, curs1); }).empty(),
                 "profesorul strain a inscris studentul");
        verifica(!mesajExceptie([&] { serviciu.inscrieStudent(profesor1, student2, 99999); }).empty(),
                 "cursul inexistent a fost acceptat");

        const auto studenti = serviciu.listeazaStudentiCurs(profesor1, curs1);
        verifica(studenti.size() == 1 && studenti.front().nume == "Popescu" &&
                     studenti.front().prenume == "Ion",
                 "lista studentilor nu contine numele corect");

        serviciu.inscrieStudent(student2, student2, curs2);
        verifica(repository.esteInscris(student2, curs2),
                 "auto-inscrierea studentului a fost stricata");

        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cout << "Test integrare inscrieri: SUCCES\n";
        return 0;
    } catch (const std::exception& eroare) {
        db.inchideConexiune();
        std::filesystem::remove(cale);
        std::cerr << eroare.what() << '\n';
        return 1;
    }
}
