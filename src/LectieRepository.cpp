#include "LectieRepository.h"

#include "ConectorBazaDate.h"
#include "ExceptieEdu.h"

#include <limits>
#include <vector>

namespace {
constexpr const char* selectLectii =
    "SELECT l.id, l.curs_id, l.proprietar_id, l.nume, l.tip, l.continut, "
    "l.dimensiune_octeti, l.creat_la, l.actualizat_la, "
    "lt.numar_cuvinte, lv.durata, lv.codec "
    "FROM lectii l "
    "LEFT JOIN lectii_text lt ON lt.lectie_id = l.id "
    "LEFT JOIN lectii_video lv ON lv.lectie_id = l.id ";

void valideazaId(int id, const std::string& denumire) {
    if (id <= 0) {
        throw ExceptieEdu(denumire + " trebuie sa fie strict pozitiv.");
    }
}

void valideazaDateLectie(const std::string& nume,
                         const std::string& tip,
                         long long dimensiuneOcteti,
                         const std::optional<long long>& numarCuvinte,
                         const std::optional<long long>& durata,
                         const std::optional<std::string>& codec) {
    if (nume.empty()) {
        throw ExceptieEdu("Numele lectiei nu poate fi gol.");
    }
    if (dimensiuneOcteti < 0) {
        throw ExceptieEdu("Dimensiunea lectiei nu poate fi negativa.");
    }

    if (tip == "text") {
        if (!numarCuvinte.has_value() || *numarCuvinte < 0) {
            throw ExceptieEdu("Lectia text necesita un numar de cuvinte nenegativ.");
        }
        if (durata.has_value() || codec.has_value()) {
            throw ExceptieEdu("Lectia text nu poate avea durata sau codec video.");
        }
        return;
    }

    if (tip == "video") {
        if (!durata.has_value() || *durata < 0) {
            throw ExceptieEdu("Lectia video necesita o durata nenegativa.");
        }
        if (!codec.has_value() || codec->empty()) {
            throw ExceptieEdu("Lectia video necesita un codec nevid.");
        }
        if (numarCuvinte.has_value()) {
            throw ExceptieEdu("Lectia video nu poate avea numar de cuvinte.");
        }
        return;
    }

    throw ExceptieEdu("Tipul lectiei trebuie sa fie text sau video.");
}

long long convertesteNumarNenegativ(const std::string& valoare,
                                    const std::string& denumire) {
    try {
        std::size_t caractereConvertite = 0;
        const long long rezultat = std::stoll(valoare, &caractereConvertite);
        if (caractereConvertite != valoare.size() || rezultat < 0) {
            throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
        }
        return rezultat;
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
    }
}

int convertesteId(const std::string& valoare, const std::string& denumire) {
    const long long rezultat = convertesteNumarNenegativ(valoare, denumire);
    if (rezultat <= 0 || rezultat > std::numeric_limits<int>::max()) {
        throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
    }
    return static_cast<int>(rezultat);
}

LectieInregistrare transformaRandul(const std::vector<std::string>& rand) {
    constexpr std::size_t numarColoaneLectie = 12;
    if (rand.size() != numarColoaneLectie) {
        throw ExceptieEdu("Randul lectiei nu contine numarul asteptat de coloane.");
    }

    LectieInregistrare lectie{
        convertesteId(rand[0], "Id-ul lectiei"),
        convertesteId(rand[1], "Id-ul cursului"),
        convertesteId(rand[2], "Id-ul proprietarului"),
        rand[3],
        rand[4],
        rand[5],
        convertesteNumarNenegativ(rand[6], "Dimensiunea lectiei"),
        rand[7],
        rand[8],
        std::nullopt,
        std::nullopt,
        std::nullopt
    };

    if (lectie.tip == "text") {
        if (rand[9].empty() || !rand[10].empty() || !rand[11].empty()) {
            throw ExceptieEdu("Specializarea lectiei text este invalida.");
        }
        lectie.numarCuvinte = convertesteNumarNenegativ(rand[9], "Numarul de cuvinte");
    } else if (lectie.tip == "video") {
        if (!rand[9].empty() || rand[10].empty() || rand[11].empty()) {
            throw ExceptieEdu("Specializarea lectiei video este invalida.");
        }
        lectie.durata = convertesteNumarNenegativ(rand[10], "Durata lectiei");
        lectie.codec = rand[11];
    } else {
        throw ExceptieEdu("Tipul lectiei citit din baza de date este invalid.");
    }

    return lectie;
}

std::vector<LectieInregistrare> transformaRezultatele(
    const std::vector<std::vector<std::string>>& rezultate) {
    std::vector<LectieInregistrare> lectii;
    lectii.reserve(rezultate.size());
    for (const auto& rand : rezultate) {
        lectii.push_back(transformaRandul(rand));
    }
    return lectii;
}

void incepeTranzactie(ConectorBazaDate& conector) {
    conector.executaInterogareParametrizata("BEGIN TRANSACTION;", {});
}

void confirmaTranzactie(ConectorBazaDate& conector) {
    conector.executaInterogareParametrizata("COMMIT;", {});
}

void anuleazaTranzactie(ConectorBazaDate& conector) noexcept {
    try {
        conector.executaInterogareParametrizata("ROLLBACK;", {});
    } catch (...) {
    }
}

void adaugaSpecializare(ConectorBazaDate& conector,
                        int lectieId,
                        const std::string& tip,
                        const std::optional<long long>& numarCuvinte,
                        const std::optional<long long>& durata,
                        const std::optional<std::string>& codec) {
    if (tip == "text") {
        conector.executaInterogareParametrizata(
            "INSERT INTO lectii_text (lectie_id, numar_cuvinte) VALUES (?, ?);",
            {std::to_string(lectieId), std::to_string(*numarCuvinte)});
    } else {
        conector.executaInterogareParametrizata(
            "INSERT INTO lectii_video (lectie_id, durata, codec) VALUES (?, ?, ?);",
            {std::to_string(lectieId), std::to_string(*durata), *codec});
    }
}
}

LectieRepository::LectieRepository(ConectorBazaDate& conector)
    : conector(conector) {
}

void LectieRepository::valideazaCursExistent(int cursId) {
    valideazaId(cursId, "Id-ul cursului");
    if (conector.executaSelectParametrizat(
            "SELECT id FROM cursuri WHERE id = ?;", {std::to_string(cursId)}).empty()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
}

void LectieRepository::valideazaProfesorExistent(int profesorId) {
    valideazaId(profesorId, "Id-ul profesorului");
    if (conector.executaSelectParametrizat(
            "SELECT utilizator_id FROM profesori WHERE utilizator_id = ?;",
            {std::to_string(profesorId)}).empty()) {
        throw ExceptieEdu("Profesorul specificat nu exista.");
    }
}

int LectieRepository::adaugaLectie(int cursId,
                                   int proprietarId,
                                   const std::string& nume,
                                   const std::string& tip,
                                   const std::string& continut,
                                   long long dimensiuneOcteti,
                                   std::optional<long long> numarCuvinte,
                                   std::optional<long long> durata,
                                   std::optional<std::string> codec) {
    valideazaCursExistent(cursId);
    valideazaProfesorExistent(proprietarId);
    valideazaDateLectie(nume, tip, dimensiuneOcteti, numarCuvinte, durata, codec);

    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "INSERT INTO lectii "
            "(curs_id, proprietar_id, nume, tip, continut, dimensiune_octeti) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {std::to_string(cursId),
             std::to_string(proprietarId),
             nume,
             tip,
             continut,
             std::to_string(dimensiuneOcteti)});

        const auto rezultat = conector.executaSelectParametrizat(
            "SELECT last_insert_rowid();", {});
        if (rezultat.size() != 1 || rezultat.front().size() != 1) {
            throw ExceptieEdu("Id-ul lectiei adaugate nu a putut fi citit.");
        }
        const int lectieId = convertesteId(rezultat.front().front(), "Id-ul lectiei");
        adaugaSpecializare(conector, lectieId, tip, numarCuvinte, durata, codec);
        confirmaTranzactie(conector);
        return lectieId;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool LectieRepository::actualizeazaLectie(int id,
                                          int cursId,
                                          int proprietarId,
                                          const std::string& nume,
                                          const std::string& tip,
                                          const std::string& continut,
                                          long long dimensiuneOcteti,
                                          std::optional<long long> numarCuvinte,
                                          std::optional<long long> durata,
                                          std::optional<std::string> codec) {
    valideazaId(id, "Id-ul lectiei");
    valideazaDateLectie(nume, tip, dimensiuneOcteti, numarCuvinte, durata, codec);
    if (!cautaDupaId(id).has_value()) {
        return false;
    }
    valideazaCursExistent(cursId);
    valideazaProfesorExistent(proprietarId);

    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "UPDATE lectii SET curs_id = ?, proprietar_id = ?, nume = ?, tip = ?, "
            "continut = ?, dimensiune_octeti = ?, actualizat_la = CURRENT_TIMESTAMP "
            "WHERE id = ?;",
            {std::to_string(cursId),
             std::to_string(proprietarId),
             nume,
             tip,
             continut,
             std::to_string(dimensiuneOcteti),
             std::to_string(id)});
        conector.executaInterogareParametrizata(
            "DELETE FROM lectii_text WHERE lectie_id = ?;", {std::to_string(id)});
        conector.executaInterogareParametrizata(
            "DELETE FROM lectii_video WHERE lectie_id = ?;", {std::to_string(id)});
        adaugaSpecializare(conector, id, tip, numarCuvinte, durata, codec);
        confirmaTranzactie(conector);
        return true;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool LectieRepository::stergeLectie(int id) {
    valideazaId(id, "Id-ul lectiei");
    return conector.executaInterogareParametrizata(
               "DELETE FROM lectii WHERE id = ?;", {std::to_string(id)}) > 0;
}

std::optional<LectieInregistrare> LectieRepository::cautaDupaId(int id) {
    valideazaId(id, "Id-ul lectiei");
    const auto rezultate = conector.executaSelectParametrizat(
        std::string(selectLectii) + "WHERE l.id = ?;", {std::to_string(id)});
    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaRandul(rezultate.front());
}

std::vector<LectieInregistrare> LectieRepository::listeazaDupaCurs(int cursId) {
    valideazaCursExistent(cursId);
    return transformaRezultatele(conector.executaSelectParametrizat(
        std::string(selectLectii) + "WHERE l.curs_id = ? ORDER BY l.id;",
        {std::to_string(cursId)}));
}

std::vector<LectieInregistrare> LectieRepository::listeazaLectii() {
    return transformaRezultatele(conector.executaSelectParametrizat(
        std::string(selectLectii) + "ORDER BY l.id;", {}));
}
