#include "UtilizatorRepository.h"

#include "ConectorBazaDate.h"
#include "ExceptieEdu.h"

#include <vector>

namespace {
void valideazaId(int id) {
    if (id <= 0) {
        throw ExceptieEdu("Id-ul utilizatorului trebuie sa fie pozitiv.");
    }
}

void valideazaEmail(const std::string& email) {
    if (email.empty()) {
        throw ExceptieEdu("Emailul utilizatorului nu poate fi gol.");
    }
}

void valideazaParola(const std::string& parola) {
    if (parola.empty()) {
        throw ExceptieEdu("Parola utilizatorului nu poate fi goala.");
    }
}

void valideazaRol(const std::string& rol) {
    if (rol != "student" && rol != "profesor" && rol != "administrator") {
        throw ExceptieEdu(
            "Rolul utilizatorului trebuie sa fie student, profesor sau administrator.");
    }
}

void valideazaDateUtilizator(const std::string& email,
                             const std::string& parola,
                             const std::string& rol) {
    valideazaEmail(email);
    valideazaParola(parola);
    valideazaRol(rol);
}

UtilizatorInregistrare transformaRandul(const std::vector<std::string>& rand) {
    constexpr std::size_t numarColoaneUtilizator = 6;
    if (rand.size() != numarColoaneUtilizator) {
        throw ExceptieEdu("Randul utilizatorului nu contine numarul asteptat de coloane.");
    }

    return {
        std::stoi(rand.at(0)),
        rand.at(1),
        rand.at(2),
        rand.at(3),
        rand.at(4),
        rand.at(5)
    };
}
}

UtilizatorRepository::UtilizatorRepository(ConectorBazaDate& conector)
    : conector(conector) {
}

int UtilizatorRepository::adaugaUtilizator(const std::string& email,
                                           const std::string& parola,
                                           const std::string& rol) {
    valideazaDateUtilizator(email, parola, rol);

    conector.executaInterogareParametrizata(
        "INSERT INTO utilizatori (email, parola, rol) VALUES (?, ?, ?);",
        {email, parola, rol});

    const auto rezultat = conector.executaSelect("SELECT last_insert_rowid();");
    return std::stoi(rezultat.at(0).at(0));
}

bool UtilizatorRepository::actualizeazaUtilizator(int id,
                                                  const std::string& email,
                                                  const std::string& parola,
                                                  const std::string& rol) {
    valideazaId(id);
    valideazaDateUtilizator(email, parola, rol);

    return conector.executaInterogareParametrizata(
               "UPDATE utilizatori SET email = ?, parola = ?, rol = ? WHERE id = ?;",
               {email, parola, rol, std::to_string(id)}) > 0;
}

bool UtilizatorRepository::stergeUtilizator(int id) {
    valideazaId(id);

    return conector.executaInterogareParametrizata(
               "DELETE FROM utilizatori WHERE id = ?;",
               {std::to_string(id)}) > 0;
}

std::optional<UtilizatorInregistrare> UtilizatorRepository::cautaDupaId(int id) {
    valideazaId(id);

    const auto rezultate = conector.executaSelectParametrizat(
        "SELECT id, email, parola, rol, data_ultima_logare, creat_la "
        "FROM utilizatori WHERE id = ?;",
        {std::to_string(id)});

    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaRandul(rezultate.front());
}

std::optional<UtilizatorInregistrare> UtilizatorRepository::cautaDupaEmail(
    const std::string& email) {
    valideazaEmail(email);

    const auto rezultate = conector.executaSelectParametrizat(
        "SELECT id, email, parola, rol, data_ultima_logare, creat_la "
        "FROM utilizatori WHERE email = ?;",
        {email});

    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaRandul(rezultate.front());
}
