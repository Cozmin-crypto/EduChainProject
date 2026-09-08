#include "EvaluareRepository.h"

#include "ConectorBazaDate.h"
#include "ExceptieEdu.h"

#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <vector>

namespace {
constexpr const char* selectEvaluari =
    "SELECT e.id, e.curs_id, e.profesor_id, e.nume, e.tip, e.limita_timp, "
    "e.este_obligatorie, e.creat_la, c.numar_intrebari, ef.pondere "
    "FROM evaluari e "
    "LEFT JOIN chestionare c ON c.evaluare_id = e.id "
    "LEFT JOIN examene_finale ef ON ef.evaluare_id = e.id ";

constexpr const char* selectIncercari =
    "SELECT id, evaluare_id, student_id, inceputa_la, finalizata_la, "
    "scor_brut, nota_finala FROM incercari_evaluare ";

void valideazaId(int id, const std::string& denumire) {
    if (id <= 0) {
        throw ExceptieEdu(denumire + " trebuie sa fie strict pozitiv.");
    }
}

long long convertesteIntregNenegativ(const std::string& valoare,
                                     const std::string& denumire) {
    try {
        std::size_t convertite = 0;
        const long long rezultat = std::stoll(valoare, &convertite);
        if (convertite != valoare.size() || rezultat < 0) {
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
    const long long rezultat = convertesteIntregNenegativ(valoare, denumire);
    if (rezultat <= 0 || rezultat > std::numeric_limits<int>::max()) {
        throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
    }
    return static_cast<int>(rezultat);
}

double convertesteReal(const std::string& valoare, const std::string& denumire) {
    try {
        std::size_t convertite = 0;
        const double rezultat = std::stod(valoare, &convertite);
        if (convertite != valoare.size() || !std::isfinite(rezultat)) {
            throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
        }
        return rezultat;
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu(denumire + " citit din baza de date este invalid.");
    }
}

std::string convertesteText(double valoare) {
    std::ostringstream flux;
    flux << std::setprecision(std::numeric_limits<double>::max_digits10) << valoare;
    return flux.str();
}

void valideazaDateEvaluare(const std::string& nume,
                           const std::string& tip,
                           long long limitaTimp,
                           const std::optional<long long>& numarIntrebari,
                           const std::optional<double>& pondere) {
    if (nume.empty()) {
        throw ExceptieEdu("Numele evaluarii nu poate fi gol.");
    }
    if (limitaTimp < 0) {
        throw ExceptieEdu("Limita de timp nu poate fi negativa.");
    }
    if (tip == "chestionar") {
        if (!numarIntrebari.has_value() || *numarIntrebari < 0) {
            throw ExceptieEdu("Chestionarul necesita un numar de intrebari nenegativ.");
        }
        if (pondere.has_value()) {
            throw ExceptieEdu("Chestionarul nu poate avea pondere de examen final.");
        }
        return;
    }
    if (tip == "examen_final") {
        if (!pondere.has_value() || !std::isfinite(*pondere) ||
            *pondere < 0.0 || *pondere > 1.0) {
            throw ExceptieEdu("Ponderea examenului final trebuie sa fie intre 0 si 1.");
        }
        if (numarIntrebari.has_value()) {
            throw ExceptieEdu("Examenul final nu poate avea numar de intrebari.");
        }
        return;
    }
    throw ExceptieEdu("Tipul evaluarii trebuie sa fie chestionar sau examen_final.");
}

EvaluareInregistrare transformaEvaluare(const std::vector<std::string>& rand) {
    if (rand.size() != 10) {
        throw ExceptieEdu("Randul evaluarii nu contine numarul asteptat de coloane.");
    }
    EvaluareInregistrare evaluare{
        convertesteId(rand[0], "Id-ul evaluarii"),
        convertesteId(rand[1], "Id-ul cursului"),
        convertesteId(rand[2], "Id-ul profesorului"),
        rand[3],
        rand[4],
        convertesteIntregNenegativ(rand[5], "Limita de timp"),
        rand[6] == "1",
        rand[7],
        std::nullopt,
        std::nullopt
    };
    if (rand[6] != "0" && rand[6] != "1") {
        throw ExceptieEdu("Indicatorul de obligativitate este invalid.");
    }
    if (evaluare.tip == "chestionar") {
        if (rand[8].empty() || !rand[9].empty()) {
            throw ExceptieEdu("Specializarea chestionarului este invalida.");
        }
        evaluare.numarIntrebari = convertesteIntregNenegativ(rand[8], "Numarul de intrebari");
    } else if (evaluare.tip == "examen_final") {
        if (!rand[8].empty() || rand[9].empty()) {
            throw ExceptieEdu("Specializarea examenului final este invalida.");
        }
        evaluare.pondere = convertesteReal(rand[9], "Ponderea");
    } else {
        throw ExceptieEdu("Tipul evaluarii citit din baza de date este invalid.");
    }
    return evaluare;
}

IntrebareChestionarInregistrare transformaIntrebare(const std::vector<std::string>& rand) {
    if (rand.size() != 6) {
        throw ExceptieEdu("Randul intrebarii nu contine numarul asteptat de coloane.");
    }
    return {convertesteId(rand[0], "Id-ul intrebarii"),
            convertesteId(rand[1], "Id-ul chestionarului"),
            rand[2],
            rand[3],
            convertesteReal(rand[4], "Punctajul maxim"),
            convertesteIntregNenegativ(rand[5], "Ordinea intrebarii")};
}

IncercareEvaluareInregistrare transformaIncercare(const std::vector<std::string>& rand) {
    if (rand.size() != 7) {
        throw ExceptieEdu("Randul incercarii nu contine numarul asteptat de coloane.");
    }
    std::optional<std::string> finalizataLa;
    if (!rand[4].empty()) {
        finalizataLa = rand[4];
    }
    return {convertesteId(rand[0], "Id-ul incercarii"),
            convertesteId(rand[1], "Id-ul evaluarii"),
            convertesteId(rand[2], "Id-ul studentului"),
            rand[3],
            finalizataLa,
            convertesteReal(rand[5], "Scorul brut"),
            convertesteReal(rand[6], "Nota finala")};
}

RaspunsChestionarInregistrare transformaRaspuns(const std::vector<std::string>& rand) {
    if (rand.size() != 4) {
        throw ExceptieEdu("Randul raspunsului nu contine numarul asteptat de coloane.");
    }
    return {convertesteId(rand[0], "Id-ul incercarii"),
            convertesteId(rand[1], "Id-ul intrebarii"),
            rand[2],
            convertesteReal(rand[3], "Punctajul obtinut")};
}

RezultatEvaluareInregistrare transformaRezultat(const std::vector<std::string>& rand) {
    if (rand.size() != 13) {
        throw ExceptieEdu("Randul rezultatului nu contine numarul asteptat de coloane.");
    }
    RezultatEvaluareInregistrare rezultat;
    rezultat.evaluareId = convertesteId(rand[0], "Id-ul evaluarii");
    rezultat.cursId = convertesteId(rand[1], "Id-ul cursului");
    rezultat.numeCurs = rand[2];
    rezultat.numeEvaluare = rand[3];
    rezultat.tipEvaluare = rand[4];
    rezultat.studentId = convertesteId(rand[5], "Id-ul studentului");
    rezultat.numeStudent = rand[6];
    rezultat.prenumeStudent = rand[7];
    rezultat.punctajMaxim = convertesteReal(rand[8], "Punctajul maxim");
    if (!rand[9].empty()) {
        rezultat.incercareId = convertesteId(rand[9], "Id-ul incercarii");
        rezultat.scorBrut = convertesteReal(rand[10], "Scorul brut");
        rezultat.notaFinala = convertesteReal(rand[11], "Nota finala");
        if (rand[12].empty()) {
            throw ExceptieEdu("Rezultatul finalizat nu contine data finalizarii.");
        }
        rezultat.finalizataLa = rand[12];
    }
    return rezultat;
}

template <typename T, typename Transformare>
std::vector<T> transformaToate(const std::vector<std::vector<std::string>>& rezultate,
                               Transformare transforma) {
    std::vector<T> inregistrari;
    inregistrari.reserve(rezultate.size());
    for (const auto& rand : rezultate) {
        inregistrari.push_back(transforma(rand));
    }
    return inregistrari;
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

int citesteUltimulId(ConectorBazaDate& conector, const std::string& denumire) {
    const auto rezultat = conector.executaSelectParametrizat("SELECT last_insert_rowid();", {});
    if (rezultat.size() != 1 || rezultat.front().size() != 1) {
        throw ExceptieEdu(denumire + " nu a putut fi citit.");
    }
    return convertesteId(rezultat.front().front(), denumire);
}

void adaugaSpecializare(ConectorBazaDate& conector,
                        int evaluareId,
                        const std::string& tip,
                        const std::optional<long long>& numarIntrebari,
                        const std::optional<double>& pondere) {
    if (tip == "chestionar") {
        conector.executaInterogareParametrizata(
            "INSERT INTO chestionare (evaluare_id, numar_intrebari) VALUES (?, ?);",
            {std::to_string(evaluareId), std::to_string(*numarIntrebari)});
    } else {
        conector.executaInterogareParametrizata(
            "INSERT INTO examene_finale (evaluare_id, pondere) VALUES (?, ?);",
            {std::to_string(evaluareId), convertesteText(*pondere)});
    }
}
}

EvaluareRepository::EvaluareRepository(ConectorBazaDate& conector)
    : conector(conector) {
}

void EvaluareRepository::valideazaCursExistent(int cursId) {
    valideazaId(cursId, "Id-ul cursului");
    if (conector.executaSelectParametrizat(
            "SELECT id FROM cursuri WHERE id = ?;", {std::to_string(cursId)}).empty()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
}

void EvaluareRepository::valideazaProfesorExistent(int profesorId) {
    valideazaId(profesorId, "Id-ul profesorului");
    if (conector.executaSelectParametrizat(
            "SELECT utilizator_id FROM profesori WHERE utilizator_id = ?;",
            {std::to_string(profesorId)}).empty()) {
        throw ExceptieEdu("Profesorul specificat nu exista.");
    }
}

void EvaluareRepository::valideazaStudentExistent(int studentId) {
    valideazaId(studentId, "Id-ul studentului");
    if (conector.executaSelectParametrizat(
            "SELECT utilizator_id FROM studenti WHERE utilizator_id = ?;",
            {std::to_string(studentId)}).empty()) {
        throw ExceptieEdu("Studentul specificat nu exista.");
    }
}

void EvaluareRepository::valideazaEvaluareExistenta(int evaluareId) {
    valideazaId(evaluareId, "Id-ul evaluarii");
    if (conector.executaSelectParametrizat(
            "SELECT id FROM evaluari WHERE id = ?;",
            {std::to_string(evaluareId)}).empty()) {
        throw ExceptieEdu("Evaluarea specificata nu exista.");
    }
}

int EvaluareRepository::adaugaEvaluare(int cursId,
                                       int profesorId,
                                       const std::string& nume,
                                       const std::string& tip,
                                       long long limitaTimp,
                                       bool esteObligatorie,
                                       std::optional<long long> numarIntrebari,
                                       std::optional<double> pondere) {
    valideazaCursExistent(cursId);
    valideazaProfesorExistent(profesorId);
    valideazaDateEvaluare(nume, tip, limitaTimp, numarIntrebari, pondere);
    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "INSERT INTO evaluari "
            "(curs_id, profesor_id, nume, tip, limita_timp, este_obligatorie) "
            "VALUES (?, ?, ?, ?, ?, ?);",
            {std::to_string(cursId), std::to_string(profesorId), nume, tip,
             std::to_string(limitaTimp), esteObligatorie ? "1" : "0"});
        const int id = citesteUltimulId(conector, "Id-ul evaluarii");
        adaugaSpecializare(conector, id, tip, numarIntrebari, pondere);
        confirmaTranzactie(conector);
        return id;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool EvaluareRepository::actualizeazaEvaluare(int id,
                                              int cursId,
                                              int profesorId,
                                              const std::string& nume,
                                              const std::string& tip,
                                              long long limitaTimp,
                                              bool esteObligatorie,
                                              std::optional<long long> numarIntrebari,
                                              std::optional<double> pondere) {
    valideazaId(id, "Id-ul evaluarii");
    valideazaDateEvaluare(nume, tip, limitaTimp, numarIntrebari, pondere);
    const auto evaluareExistenta = cautaDupaId(id);
    if (!evaluareExistenta.has_value()) {
        return false;
    }
    valideazaCursExistent(cursId);
    valideazaProfesorExistent(profesorId);
    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "UPDATE evaluari SET curs_id = ?, profesor_id = ?, nume = ?, tip = ?, "
            "limita_timp = ?, este_obligatorie = ? WHERE id = ?;",
            {std::to_string(cursId), std::to_string(profesorId), nume, tip,
             std::to_string(limitaTimp), esteObligatorie ? "1" : "0", std::to_string(id)});
        if (evaluareExistenta->tip == tip && tip == "chestionar") {
            conector.executaInterogareParametrizata(
                "UPDATE chestionare SET numar_intrebari = ? WHERE evaluare_id = ?;",
                {std::to_string(*numarIntrebari), std::to_string(id)});
        } else if (evaluareExistenta->tip == tip && tip == "examen_final") {
            conector.executaInterogareParametrizata(
                "UPDATE examene_finale SET pondere = ? WHERE evaluare_id = ?;",
                {convertesteText(*pondere), std::to_string(id)});
        } else {
            conector.executaInterogareParametrizata(
                "DELETE FROM chestionare WHERE evaluare_id = ?;", {std::to_string(id)});
            conector.executaInterogareParametrizata(
                "DELETE FROM examene_finale WHERE evaluare_id = ?;", {std::to_string(id)});
            adaugaSpecializare(conector, id, tip, numarIntrebari, pondere);
        }
        confirmaTranzactie(conector);
        return true;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool EvaluareRepository::stergeEvaluare(int id) {
    valideazaId(id, "Id-ul evaluarii");
    return conector.executaInterogareParametrizata(
               "DELETE FROM evaluari WHERE id = ?;", {std::to_string(id)}) > 0;
}

std::optional<EvaluareInregistrare> EvaluareRepository::cautaDupaId(int id) {
    valideazaId(id, "Id-ul evaluarii");
    const auto rezultate = conector.executaSelectParametrizat(
        std::string(selectEvaluari) + "WHERE e.id = ?;", {std::to_string(id)});
    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaEvaluare(rezultate.front());
}

std::vector<EvaluareInregistrare> EvaluareRepository::listeazaDupaCurs(int cursId) {
    valideazaCursExistent(cursId);
    return transformaToate<EvaluareInregistrare>(
        conector.executaSelectParametrizat(
            std::string(selectEvaluari) + "WHERE e.curs_id = ? ORDER BY e.id;",
            {std::to_string(cursId)}),
        transformaEvaluare);
}

int EvaluareRepository::adaugaIntrebare(int chestionarId,
                                        const std::string& enunt,
                                        const std::string& raspunsCorect,
                                        double punctajMaxim,
                                        long long ordine) {
    valideazaEvaluareExistenta(chestionarId);
    if (enunt.empty() || raspunsCorect.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw ExceptieEdu("Enuntul si raspunsul corect nu pot fi goale.");
    }
    if (!std::isfinite(punctajMaxim) || punctajMaxim < 0.0 || ordine < 0) {
        throw ExceptieEdu("Punctajul si ordinea intrebarii trebuie sa fie nenegative.");
    }
    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "INSERT INTO intrebari_chestionar "
            "(chestionar_id, enunt, raspuns_corect, punctaj_maxim, ordine) VALUES (?, ?, ?, ?, ?);",
            {std::to_string(chestionarId), enunt, raspunsCorect, convertesteText(punctajMaxim),
             std::to_string(ordine)});
        const int id = citesteUltimulId(conector, "Id-ul intrebarii");
        conector.executaInterogareParametrizata(
            "UPDATE chestionare SET numar_intrebari = numar_intrebari + 1 "
            "WHERE evaluare_id = ?;", {std::to_string(chestionarId)});
        confirmaTranzactie(conector);
        return id;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool EvaluareRepository::actualizeazaIntrebare(int intrebareId,
                                               const std::string& enunt,
                                               const std::string& raspunsCorect,
                                               double punctajMaxim,
                                               long long ordine) {
    valideazaId(intrebareId, "Id-ul intrebarii");
    if (enunt.empty() || raspunsCorect.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw ExceptieEdu("Enuntul si raspunsul corect nu pot fi goale.");
    }
    if (!std::isfinite(punctajMaxim) || punctajMaxim < 0.0 || ordine < 0) {
        throw ExceptieEdu("Punctajul si ordinea intrebarii trebuie sa fie nenegative.");
    }
    return conector.executaInterogareParametrizata(
               "UPDATE intrebari_chestionar "
               "SET enunt = ?, raspuns_corect = ?, punctaj_maxim = ?, ordine = ? WHERE id = ?;",
               {enunt,
                raspunsCorect,
                convertesteText(punctajMaxim),
                std::to_string(ordine),
                std::to_string(intrebareId)}) > 0;
}

bool EvaluareRepository::stergeIntrebare(int intrebareId) {
    valideazaId(intrebareId, "Id-ul intrebarii");
    const auto rezultat = conector.executaSelectParametrizat(
        "SELECT chestionar_id FROM intrebari_chestionar WHERE id = ?;",
        {std::to_string(intrebareId)});
    if (rezultat.empty()) {
        return false;
    }
    const int chestionarId = convertesteId(rezultat.front().front(), "Id-ul chestionarului");
    incepeTranzactie(conector);
    try {
        conector.executaInterogareParametrizata(
            "DELETE FROM intrebari_chestionar WHERE id = ?;", {std::to_string(intrebareId)});
        conector.executaInterogareParametrizata(
            "UPDATE chestionare SET numar_intrebari = "
            "CASE WHEN numar_intrebari > 0 THEN numar_intrebari - 1 ELSE 0 END "
            "WHERE evaluare_id = ?;", {std::to_string(chestionarId)});
        confirmaTranzactie(conector);
        return true;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

std::vector<IntrebareChestionarInregistrare> EvaluareRepository::listeazaIntrebari(
    int chestionarId) {
    valideazaEvaluareExistenta(chestionarId);
    return transformaToate<IntrebareChestionarInregistrare>(
        conector.executaSelectParametrizat(
            "SELECT id, chestionar_id, enunt, raspuns_corect, punctaj_maxim, ordine "
            "FROM intrebari_chestionar WHERE chestionar_id = ? ORDER BY ordine;",
            {std::to_string(chestionarId)}),
        transformaIntrebare);
}

std::optional<IntrebareChestionarInregistrare>
EvaluareRepository::cautaIntrebareDupaId(int intrebareId) {
    valideazaId(intrebareId, "Id-ul intrebarii");
    const auto randuri = conector.executaSelectParametrizat(
        "SELECT id, chestionar_id, enunt, raspuns_corect, punctaj_maxim, ordine "
        "FROM intrebari_chestionar WHERE id = ?;", {std::to_string(intrebareId)});
    if (randuri.empty()) return std::nullopt;
    return transformaIntrebare(randuri.front());
}

int EvaluareRepository::adaugaIncercare(int evaluareId, int studentId) {
    valideazaId(evaluareId, "Id-ul evaluarii");
    if (!cautaDupaId(evaluareId).has_value()) {
        throw ExceptieEdu("Evaluarea specificata nu exista.");
    }
    valideazaStudentExistent(studentId);
    incepeTranzactie(conector);
    try {
        const auto existenta = conector.executaSelectParametrizat(
            "SELECT id FROM incercari_evaluare WHERE evaluare_id = ? AND student_id = ? "
            "LIMIT 1;",
            {std::to_string(evaluareId), std::to_string(studentId)});
        if (!existenta.empty()) {
            throw ExceptieEdu("EVALUARE_DEJA_SUSTINUTA");
        }
        conector.executaInterogareParametrizata(
            "INSERT INTO incercari_evaluare (evaluare_id, student_id) VALUES (?, ?);",
            {std::to_string(evaluareId), std::to_string(studentId)});
        const int id = citesteUltimulId(conector, "Id-ul incercarii");
        confirmaTranzactie(conector);
        return id;
    } catch (...) {
        anuleazaTranzactie(conector);
        throw;
    }
}

bool EvaluareRepository::finalizeazaIncercare(int incercareId,
                                              double scorBrut,
                                              double notaFinala) {
    valideazaId(incercareId, "Id-ul incercarii");
    if (!std::isfinite(scorBrut) || scorBrut < 0.0 ||
        !std::isfinite(notaFinala) || notaFinala < 0.0 || notaFinala > 10.0) {
        throw ExceptieEdu("Scorul sau nota finala este invalida.");
    }
    return conector.executaInterogareParametrizata(
               "UPDATE incercari_evaluare SET finalizata_la = CURRENT_TIMESTAMP, "
               "scor_brut = ?, nota_finala = ? WHERE id = ? AND finalizata_la IS NULL;",
               {convertesteText(scorBrut), convertesteText(notaFinala),
                std::to_string(incercareId)}) > 0;
}

std::optional<IncercareEvaluareInregistrare> EvaluareRepository::cautaIncercareDupaId(
    int incercareId) {
    valideazaId(incercareId, "Id-ul incercarii");
    const auto rezultate = conector.executaSelectParametrizat(
        std::string(selectIncercari) + "WHERE id = ?;", {std::to_string(incercareId)});
    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaIncercare(rezultate.front());
}

std::optional<IncercareEvaluareInregistrare>
EvaluareRepository::cautaIncercareDupaEvaluareStudent(int evaluareId, int studentId) {
    valideazaId(evaluareId, "Id-ul evaluarii");
    valideazaId(studentId, "Id-ul studentului");
    const auto rezultate = conector.executaSelectParametrizat(
        std::string(selectIncercari) +
            "WHERE evaluare_id = ? AND student_id = ? ORDER BY id DESC LIMIT 1;",
        {std::to_string(evaluareId), std::to_string(studentId)});
    if (rezultate.empty()) {
        return std::nullopt;
    }
    return transformaIncercare(rezultate.front());
}

std::vector<IncercareEvaluareInregistrare> EvaluareRepository::listeazaIncercari(
    int evaluareId) {
    valideazaId(evaluareId, "Id-ul evaluarii");
    if (!cautaDupaId(evaluareId).has_value()) {
        throw ExceptieEdu("Evaluarea specificata nu exista.");
    }
    return transformaToate<IncercareEvaluareInregistrare>(
        conector.executaSelectParametrizat(
            std::string(selectIncercari) + "WHERE evaluare_id = ? ORDER BY id;",
            {std::to_string(evaluareId)}),
        transformaIncercare);
}

std::vector<RezultatEvaluareInregistrare> EvaluareRepository::listeazaRezultateStudent(
    int studentId) {
    valideazaStudentExistent(studentId);
    const std::string selectRezultate =
        "SELECT e.id, c.id, c.nume, e.nume, e.tip, u.id, u.nume, u.prenume, "
        "COALESCE((SELECT SUM(q.punctaj_maxim) FROM intrebari_chestionar q "
        "WHERE q.chestionar_id = e.id), 0), "
        "i.id, i.scor_brut, i.nota_finala, i.finalizata_la "
        "FROM inscrieri_curs ic "
        "JOIN cursuri c ON c.id = ic.curs_id "
        "JOIN evaluari e ON e.curs_id = c.id "
        "JOIN utilizatori u ON u.id = ic.student_id "
        "LEFT JOIN incercari_evaluare i ON i.id = ("
        "SELECT MAX(i2.id) FROM incercari_evaluare i2 "
        "WHERE i2.evaluare_id = e.id AND i2.student_id = ic.student_id "
        "AND i2.finalizata_la IS NOT NULL) "
        "WHERE ic.student_id = ? ORDER BY c.nume, e.nume, e.id;";
    return transformaToate<RezultatEvaluareInregistrare>(
        conector.executaSelectParametrizat(selectRezultate, {std::to_string(studentId)}),
        transformaRezultat);
}

std::vector<RezultatEvaluareInregistrare> EvaluareRepository::listeazaRezultateEvaluare(
    int evaluareId) {
    valideazaEvaluareExistenta(evaluareId);
    const std::string selectRezultate =
        "SELECT e.id, c.id, c.nume, e.nume, e.tip, u.id, u.nume, u.prenume, "
        "COALESCE((SELECT SUM(q.punctaj_maxim) FROM intrebari_chestionar q "
        "WHERE q.chestionar_id = e.id), 0), "
        "i.id, i.scor_brut, i.nota_finala, i.finalizata_la "
        "FROM evaluari e "
        "JOIN cursuri c ON c.id = e.curs_id "
        "JOIN inscrieri_curs ic ON ic.curs_id = c.id "
        "JOIN utilizatori u ON u.id = ic.student_id "
        "LEFT JOIN incercari_evaluare i ON i.id = ("
        "SELECT MAX(i2.id) FROM incercari_evaluare i2 "
        "WHERE i2.evaluare_id = e.id AND i2.student_id = ic.student_id "
        "AND i2.finalizata_la IS NOT NULL) "
        "WHERE e.id = ? ORDER BY u.nume, u.prenume, u.id;";
    return transformaToate<RezultatEvaluareInregistrare>(
        conector.executaSelectParametrizat(selectRezultate, {std::to_string(evaluareId)}),
        transformaRezultat);
}

void EvaluareRepository::salveazaRaspuns(int incercareId,
                                         int intrebareId,
                                         const std::string& raspuns,
                                         double punctajObtinut) {
    valideazaId(incercareId, "Id-ul incercarii");
    valideazaId(intrebareId, "Id-ul intrebarii");
    if (raspuns.empty()) {
        throw ExceptieEdu("Raspunsul nu poate fi gol.");
    }
    if (!std::isfinite(punctajObtinut) || punctajObtinut < 0.0) {
        throw ExceptieEdu("Punctajul obtinut trebuie sa fie nenegativ.");
    }
    const auto relatie = conector.executaSelectParametrizat(
        "SELECT ie.id FROM incercari_evaluare ie "
        "JOIN intrebari_chestionar iq ON iq.chestionar_id = ie.evaluare_id "
        "WHERE ie.id = ? AND iq.id = ?;",
        {std::to_string(incercareId), std::to_string(intrebareId)});
    if (relatie.empty()) {
        throw ExceptieEdu("Incercarea si intrebarea nu apartin aceluiasi chestionar.");
    }
    conector.executaInterogareParametrizata(
        "INSERT INTO raspunsuri_chestionar "
        "(incercare_id, intrebare_id, raspuns, punctaj_obtinut) VALUES (?, ?, ?, ?) "
        "ON CONFLICT(incercare_id, intrebare_id) DO UPDATE SET "
        "raspuns = excluded.raspuns, punctaj_obtinut = excluded.punctaj_obtinut;",
        {std::to_string(incercareId), std::to_string(intrebareId), raspuns,
         convertesteText(punctajObtinut)});
}

std::vector<RaspunsChestionarInregistrare> EvaluareRepository::listeazaRaspunsuri(
    int incercareId) {
    valideazaId(incercareId, "Id-ul incercarii");
    if (!cautaIncercareDupaId(incercareId).has_value()) {
        throw ExceptieEdu("Incercarea specificata nu exista.");
    }
    return transformaToate<RaspunsChestionarInregistrare>(
        conector.executaSelectParametrizat(
            "SELECT incercare_id, intrebare_id, raspuns, punctaj_obtinut "
            "FROM raspunsuri_chestionar WHERE incercare_id = ? ORDER BY intrebare_id;",
            {std::to_string(incercareId)}),
        transformaRaspuns);
}
