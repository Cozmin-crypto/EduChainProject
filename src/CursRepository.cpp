#include "CursRepository.h"

#include "ConectorBazaDate.h"
#include "ExceptieEdu.h"

#include <limits>
#include <vector>

namespace {
void valideazaId(int id, const std::string& denumire) {
    if (id <= 0) {
        throw ExceptieEdu(denumire + " trebuie sa fie pozitiv.");
    }
}

void valideazaNume(const std::string& nume) {
    if (nume.empty()) {
        throw ExceptieEdu("Numele cursului nu poate fi gol.");
    }
}

void valideazaParinte(std::optional<int> parinteId) {
    if (parinteId.has_value()) {
        valideazaId(*parinteId, "Id-ul cursului parinte");
    }
}

int convertesteId(const std::string& valoare, const std::string& denumire) {
    try {
        std::size_t caractereConvertite = 0;
        const long long rezultat = std::stoll(valoare, &caractereConvertite);
        if (caractereConvertite != valoare.size() ||
            rezultat <= 0 ||
            rezultat > std::numeric_limits<int>::max()) {
            throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
        }
        return static_cast<int>(rezultat);
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
    }
}

CursInregistrare transformaRandul(const std::vector<std::string>& rand) {
    constexpr std::size_t numarColoaneCurs = 6;
    if (rand.size() != numarColoaneCurs) {
        throw ExceptieEdu("Randul cursului nu contine numarul asteptat de coloane.");
    }

    std::optional<int> parinteId;
    if (!rand[2].empty()) {
        parinteId = convertesteId(rand[2], "Id-ul cursului parinte");
    }

    return {
        convertesteId(rand[0], "Id-ul cursului"),
        rand[1],
        parinteId,
        convertesteId(rand[3], "Id-ul proprietarului"),
        rand[4],
        rand[5]
    };
}

std::vector<CursInregistrare> transformaRezultatele(
    const std::vector<std::vector<std::string>>& rezultate) {
    std::vector<CursInregistrare> cursuri;
    cursuri.reserve(rezultate.size());
    for (const auto& rand : rezultate) {
        cursuri.push_back(transformaRandul(rand));
    }
    return cursuri;
}
}

CursRepository::CursRepository(ConectorBazaDate& conector)
    : conector(conector) {
}

void CursRepository::valideazaProfesorExistent(int profesorId) {
    valideazaId(profesorId, "Id-ul profesorului");
    const auto rezultate = conector.executaSelectParametrizat(
        "SELECT utilizator_id FROM profesori WHERE utilizator_id = ?;",
        {std::to_string(profesorId)});
    if (rezultate.empty()) {
        throw ExceptieEdu("Profesorul specificat nu exista.");
    }
}

int CursRepository::adaugaCurs(const std::string& nume,
                               std::optional<int> parinteId,
                               int proprietarId) {
    valideazaNume(nume);
    valideazaParinte(parinteId);
    valideazaProfesorExistent(proprietarId);

    if (parinteId.has_value()) {
        conector.executaInterogareParametrizata(
            "INSERT INTO cursuri (nume, parinte_id, proprietar_id) VALUES (?, ?, ?);",
            {nume, std::to_string(*parinteId), std::to_string(proprietarId)});
    } else {
        conector.executaInterogareParametrizata(
            "INSERT INTO cursuri (nume, parinte_id, proprietar_id) VALUES (?, NULL, ?);",
            {nume, std::to_string(proprietarId)});
    }

    const auto rezultat = conector.executaSelectParametrizat(
        "SELECT last_insert_rowid();", {});
    if (rezultat.size() != 1 || rezultat.front().size() != 1) {
        throw ExceptieEdu("Id-ul cursului adaugat nu a putut fi citit.");
    }
    return convertesteId(rezultat.front().front(), "Id-ul cursului");
}

bool CursRepository::actualizeazaCurs(int id,
                                      const std::string& nume,
                                      std::optional<int> parinteId,
                                      int proprietarId) {
    valideazaId(id, "Id-ul cursului");
    valideazaNume(nume);
    valideazaParinte(parinteId);
    valideazaProfesorExistent(proprietarId);

    if (parinteId.has_value()) {
        return conector.executaInterogareParametrizata(
                   "UPDATE cursuri SET nume = ?, parinte_id = ?, proprietar_id = ?, "
                   "actualizat_la = CURRENT_TIMESTAMP WHERE id = ?;",
                   {nume,
                    std::to_string(*parinteId),
                    std::to_string(proprietarId),
                    std::to_string(id)}) > 0;
    }

    return conector.executaInterogareParametrizata(
               "UPDATE cursuri SET nume = ?, parinte_id = NULL, proprietar_id = ?, "
               "actualizat_la = CURRENT_TIMESTAMP WHERE id = ?;",
               {nume, std::to_string(proprietarId), std::to_string(id)}) > 0;
}

bool CursRepository::stergeCurs(int id) {
    valideazaId(id, "Id-ul cursului");
    return conector.executaInterogareParametrizata(
               "DELETE FROM cursuri WHERE id = ?;",
               {std::to_string(id)}) > 0;
}

std::optional<CursInregistrare> CursRepository::cautaDupaId(int id) {
    valideazaId(id, "Id-ul cursului");
    const auto rezultate = conector.executaSelectParametrizat(
        "SELECT id, nume, parinte_id, proprietar_id, creat_la, actualizat_la "
        "FROM cursuri WHERE id = ?;",
        {std::to_string(id)});
    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaRandul(rezultate.front());
}

std::vector<CursInregistrare> CursRepository::listeazaCursuri() {
    return transformaRezultatele(conector.executaSelectParametrizat(
        "SELECT id, nume, parinte_id, proprietar_id, creat_la, actualizat_la "
        "FROM cursuri ORDER BY id;",
        {}));
}

std::vector<CursInregistrare> CursRepository::listeazaDupaProfesor(int profesorId) {
    valideazaProfesorExistent(profesorId);
    return transformaRezultatele(conector.executaSelectParametrizat(
        "SELECT id, nume, parinte_id, proprietar_id, creat_la, actualizat_la "
        "FROM cursuri WHERE proprietar_id = ? ORDER BY id;",
        {std::to_string(profesorId)}));
}
