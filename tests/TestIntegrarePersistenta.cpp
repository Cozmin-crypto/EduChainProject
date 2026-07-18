#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "EvaluareRepository.h"
#include "LectieRepository.h"
#include "UtilizatorRepository.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

namespace {
void verifica(bool conditie, const std::string& mesaj) {
    if (!conditie) {
        throw std::runtime_error("Verificare esuata: " + mesaj);
    }
}

std::filesystem::path creeazaCaleTemporara() {
    const auto marcaj = std::chrono::high_resolution_clock::now()
                            .time_since_epoch()
                            .count();
    return std::filesystem::temp_directory_path() /
           ("educhain_integrare_" + std::to_string(marcaj) + ".db");
}

void creeazaSpecializareaProfesor(ConectorBazaDate& conector, int utilizatorId) {
    // Nu exista inca un API public pentru tabelele personal si profesori.
    // Inserarile sunt obligatorii pentru lantul de chei straine din schema.sql.
    conector.executaInterogareParametrizata(
        "INSERT INTO personal (utilizator_id, departament, data_angajarii) "
        "VALUES (?, ?, ?);",
        {std::to_string(utilizatorId), "Informatica", "2026-07-18"});
    conector.executaInterogareParametrizata(
        "INSERT INTO profesori (utilizator_id) VALUES (?);",
        {std::to_string(utilizatorId)});
}

void creeazaSpecializareaStudent(ConectorBazaDate& conector, int utilizatorId) {
    // UtilizatorRepository gestioneaza tabela de baza, dar nu expune tabela studenti.
    conector.executaInterogareParametrizata(
        "INSERT INTO studenti (utilizator_id) VALUES (?);",
        {std::to_string(utilizatorId)});
}

void verificaTabelaFaraRanduri(ConectorBazaDate& conector,
                               const std::string& interogare,
                               int id,
                               const std::string& mesaj) {
    // Aceste verificari observa direct cascadele din tabele fara API public de cautare.
    const auto rezultate = conector.executaSelectParametrizat(
        interogare, {std::to_string(id)});
    verifica(rezultate.empty(), mesaj);
}
}

int main() {
    const std::filesystem::path caleBazaDate = creeazaCaleTemporara();
    ConectorBazaDate conector;

    try {
        std::filesystem::remove(caleBazaDate);
        conector.deschideConexiune(caleBazaDate.string());
        verifica(conector.esteConectat(), "conexiunea temporara nu este deschisa");

        UtilizatorRepository utilizatori(conector);
        CursRepository cursuri(conector);
        LectieRepository lectii(conector);
        EvaluareRepository evaluari(conector);

        const int profesorId = utilizatori.adaugaUtilizator(
            "profesor.integrare@example.ro", "parola-profesor", "profesor");
        creeazaSpecializareaProfesor(conector, profesorId);
        verifica(utilizatori.cautaDupaId(profesorId)->rol == "profesor",
                 "utilizatorul profesor nu a fost citit corect");

        const int cursId = cursuri.adaugaCurs(
            "Programare avansata", std::nullopt, profesorId);
        verifica(cursuri.cautaDupaId(cursId)->proprietarId == profesorId,
                 "cursul nu apartine profesorului");

        const std::string continutText =
            "Introducere in C++: text cu apostrof ' si secventa SQL --";
        const int lectieId = lectii.adaugaLectie(
            cursId,
            profesorId,
            "Lectie introductiva",
            "text",
            continutText,
            static_cast<long long>(continutText.size()),
            9);
        const auto lectieText = lectii.cautaDupaId(lectieId);
        verifica(lectieText.has_value() && lectieText->tip == "text",
                 "lectia text nu a fost persistata");
        verifica(lectieText->continut == continutText && lectieText->numarCuvinte == 9,
                 "datele specializarii text sunt incorecte");

        verifica(lectii.actualizeazaLectie(
                     lectieId,
                     cursId,
                     profesorId,
                     "Lectie demonstrativa",
                     "video",
                     "lectie.mp4",
                     4096,
                     std::nullopt,
                     180,
                     std::string("h264")),
                 "actualizarea lectiei a esuat");
        const auto lectieVideo = lectii.cautaDupaId(lectieId);
        verifica(lectieVideo->tip == "video" && !lectieVideo->numarCuvinte.has_value(),
                 "specializarea text nu a fost inlocuita");
        verifica(lectieVideo->durata == 180 && lectieVideo->codec == "h264",
                 "specializarea video este incorecta");

        const int chestionarId = evaluari.adaugaEvaluare(
            cursId, profesorId, "Evaluare C++", "chestionar", 30, true, 0);
        const int intrebareId = evaluari.adaugaIntrebare(
            chestionarId,
            "Ce afiseaza expresia cout << \"' OR 1=1 --\"?",
            10.0,
            0);
        verifica(evaluari.listeazaIntrebari(chestionarId).size() == 1,
                 "intrebarea nu apare in listare");

        const int studentId = utilizatori.adaugaUtilizator(
            "student.integrare@example.ro", "parola-student", "student");
        creeazaSpecializareaStudent(conector, studentId);

        const int incercareId = evaluari.adaugaIncercare(chestionarId, studentId);
        const std::string raspuns = "Afiseaza literal textul ' OR 1=1 --";
        evaluari.salveazaRaspuns(incercareId, intrebareId, raspuns, 10.0);
        verifica(evaluari.finalizeazaIncercare(incercareId, 10.0, 10.0),
                 "finalizarea incercarii a esuat");

        const auto incercare = evaluari.cautaIncercareDupaId(incercareId);
        verifica(incercare.has_value() && incercare->finalizataLa.has_value(),
                 "incercarea nu este marcata ca finalizata");
        verifica(incercare->scorBrut == 10.0 && incercare->notaFinala == 10.0,
                 "rezultatul final este incorect");
        const auto raspunsuri = evaluari.listeazaRaspunsuri(incercareId);
        verifica(raspunsuri.size() == 1 && raspunsuri.front().raspuns == raspuns,
                 "raspunsul salvat nu a fost citit corect");

        verifica(cursuri.listeazaDupaProfesor(profesorId).size() == 1,
                 "listarea cursurilor profesorului este incorecta");
        verifica(lectii.listeazaDupaCurs(cursId).size() == 1,
                 "listarea lectiilor cursului este incorecta");
        verifica(evaluari.listeazaDupaCurs(cursId).size() == 1,
                 "listarea evaluarilor cursului este incorecta");
        verifica(evaluari.listeazaIncercari(chestionarId).size() == 1,
                 "listarea incercarilor este incorecta");

        // Stergerea utilizatorului de baza elimina studentul, incercarea si raspunsul.
        verifica(utilizatori.stergeUtilizator(studentId),
                 "stergerea studentului a esuat");
        verifica(!evaluari.cautaIncercareDupaId(incercareId).has_value(),
                 "ON DELETE CASCADE nu a eliminat incercarea studentului");
        verificaTabelaFaraRanduri(
            conector,
            "SELECT utilizator_id FROM studenti WHERE utilizator_id = ?;",
            studentId,
            "ON DELETE CASCADE nu a eliminat specializarea studentului");
        verificaTabelaFaraRanduri(
            conector,
            "SELECT incercare_id FROM raspunsuri_chestionar WHERE incercare_id = ?;",
            incercareId,
            "ON DELETE CASCADE nu a eliminat raspunsul");

        // Stergerea cursului elimina lectia, evaluarea, chestionarul si intrebarile.
        verifica(cursuri.stergeCurs(cursId), "stergerea cursului a esuat");
        verifica(!lectii.cautaDupaId(lectieId).has_value(),
                 "ON DELETE CASCADE nu a eliminat lectia");
        verifica(!evaluari.cautaDupaId(chestionarId).has_value(),
                 "ON DELETE CASCADE nu a eliminat evaluarea");
        verificaTabelaFaraRanduri(
            conector,
            "SELECT evaluare_id FROM chestionare WHERE evaluare_id = ?;",
            chestionarId,
            "ON DELETE CASCADE nu a eliminat chestionarul");
        verificaTabelaFaraRanduri(
            conector,
            "SELECT id FROM intrebari_chestionar WHERE id = ?;",
            intrebareId,
            "ON DELETE CASCADE nu a eliminat intrebarea");

        verifica(utilizatori.stergeUtilizator(profesorId),
                 "stergerea profesorului dupa curs a esuat");
        verificaTabelaFaraRanduri(
            conector,
            "SELECT utilizator_id FROM profesori WHERE utilizator_id = ?;",
            profesorId,
            "ON DELETE CASCADE nu a eliminat specializarea profesorului");

        conector.inchideConexiune();
        verifica(!conector.esteConectat(), "conexiunea nu a fost inchisa");
        verifica(std::filesystem::remove(caleBazaDate),
                 "baza de date temporara nu a fost eliminata");

        std::cout << "Testul de integrare al persistentei a trecut.\n";
        return 0;
    } catch (const std::exception& exceptie) {
        if (conector.esteConectat()) {
            try {
                conector.inchideConexiune();
            } catch (...) {
            }
        }
        std::error_code eroareStergere;
        std::filesystem::remove(caleBazaDate, eroareStergere);
        std::cerr << exceptie.what() << '\n';
        return 1;
    }
}
