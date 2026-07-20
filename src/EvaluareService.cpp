#include "EvaluareService.h"

#include "CursRepository.h"
#include "InscriereService.h"
#include "ExceptieEdu.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace {
std::string normalizeazaRaspuns(std::string text) {
    const auto inceput = std::find_if_not(text.begin(), text.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto sfarsit = std::find_if_not(text.rbegin(), text.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (inceput >= sfarsit) return {};
    std::string rezultat(inceput, sfarsit);
    std::transform(rezultat.begin(), rezultat.end(), rezultat.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return rezultat;
}
}
EvaluareService::EvaluareService(EvaluareRepository&e,CursRepository&c,UtilizatorRepository&u,InscriereService&i):evaluari(e),cursuri(c),reguli(u),inscrieri(&i){}
std::vector<EvaluareInregistrare> EvaluareService::listeazaDupaCurs(int actorId,int cursId){if(!inscrieri)throw ExceptieEdu("Serviciul de inscrieri nu este disponibil.");inscrieri->verificaAccesStudentLaCurs(actorId,cursId);return evaluari.listeazaDupaCurs(cursId);}

EvaluareService::EvaluareService(EvaluareRepository& evaluari,
                                 CursRepository& cursuri,
                                 UtilizatorRepository& utilizatori)
    : evaluari(evaluari), cursuri(cursuri), reguli(utilizatori) {
}
std::optional<EvaluareInregistrare> EvaluareService::obtineEvaluare(int actorId,int id){const auto e=evaluari.cautaDupaId(id);if(e){if(!inscrieri)throw ExceptieEdu("Serviciul de inscrieri nu este disponibil.");inscrieri->verificaAccesStudentLaCurs(actorId,e->cursId);}return e;}

CursInregistrare EvaluareService::obtineCursExistent(int cursId) {
    const auto curs = cursuri.cautaDupaId(cursId);
    if (!curs.has_value()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
    return *curs;
}

EvaluareInregistrare EvaluareService::obtineEvaluareExistenta(int evaluareId) {
    const auto evaluare = evaluari.cautaDupaId(evaluareId);
    if (!evaluare.has_value()) {
        throw ExceptieEdu("Evaluarea specificata nu exista.");
    }
    return *evaluare;
}

void EvaluareService::verificaAdministrareEvaluare(
    int actorId,
    const EvaluareInregistrare& evaluare) {
    reguli.verificaAdministratorSauProprietar(
        actorId, obtineCursExistent(evaluare.cursId));
}

IncercareEvaluareInregistrare EvaluareService::obtineIncercareStudent(
    int actorId,
    int incercareId) {
    reguli.verificaStudent(actorId);
    const auto incercare = evaluari.cautaIncercareDupaId(incercareId);
    if (!incercare.has_value()) {
        throw ExceptieEdu("Incercarea specificata nu exista.");
    }
    if (incercare->studentId != actorId) {
        throw ExceptieEdu("Studentul nu poate accesa incercarea altui student.");
    }
    return *incercare;
}

std::vector<EvaluareInregistrare> EvaluareService::listeazaDupaCurs(int cursId) {
    return evaluari.listeazaDupaCurs(cursId);
}

std::optional<EvaluareInregistrare> EvaluareService::obtineEvaluare(int evaluareId) {
    return evaluari.cautaDupaId(evaluareId);
}

int EvaluareService::creeazaEvaluare(const CerereSalvareEvaluare& cerere) {
    const auto curs = obtineCursExistent(cerere.cursId);
    reguli.verificaAdministratorSauProprietar(cerere.actorId, curs);
    return evaluari.adaugaEvaluare(
        curs.id, curs.proprietarId, cerere.nume, cerere.tip, cerere.limitaTimp,
        cerere.esteObligatorie, cerere.numarIntrebari, cerere.pondere);
}

bool EvaluareService::actualizeazaEvaluare(
    const CerereActualizareEvaluare& cerere) {
    const auto existenta = obtineEvaluareExistenta(cerere.evaluareId);
    verificaAdministrareEvaluare(cerere.actorId, existenta);
    const auto cursDestinatie = obtineCursExistent(cerere.cursId);
    reguli.verificaAdministratorSauProprietar(cerere.actorId, cursDestinatie);
    return evaluari.actualizeazaEvaluare(
        existenta.id, cursDestinatie.id, cursDestinatie.proprietarId,
        cerere.nume, cerere.tip, cerere.limitaTimp, cerere.esteObligatorie,
        cerere.numarIntrebari, cerere.pondere);
}

bool EvaluareService::stergeEvaluare(int actorId, int evaluareId) {
    const auto evaluare = obtineEvaluareExistenta(evaluareId);
    verificaAdministrareEvaluare(actorId, evaluare);
    return evaluari.stergeEvaluare(evaluareId);
}

int EvaluareService::adaugaIntrebare(const CerereIntrebare& cerere) {
    const auto evaluare = obtineEvaluareExistenta(cerere.chestionarId);
    if (evaluare.tip != "chestionar") {
        throw ExceptieEdu("Intrebarile pot fi adaugate doar unui chestionar.");
    }
    verificaAdministrareEvaluare(cerere.actorId, evaluare);
    return evaluari.adaugaIntrebare(
        evaluare.id, cerere.enunt, cerere.raspunsCorect,
        cerere.punctajMaxim, cerere.ordine);
}

bool EvaluareService::actualizeazaIntrebare(
    const CerereActualizareIntrebare& cerere) {
    const auto evaluare = obtineEvaluareExistenta(cerere.chestionarId);
    if (evaluare.tip != "chestionar") {
        throw ExceptieEdu("Intrebarea trebuie sa apartina unui chestionar.");
    }
    verificaAdministrareEvaluare(cerere.actorId, evaluare);
    const auto intrebari = evaluari.listeazaIntrebari(cerere.chestionarId);
    const bool apartineChestionarului = std::any_of(
        intrebari.begin(), intrebari.end(), [&](const auto& intrebare) {
            return intrebare.id == cerere.intrebareId;
        });
    if (!apartineChestionarului) {
        throw ExceptieEdu("Intrebarea nu apartine chestionarului specificat.");
    }
    return evaluari.actualizeazaIntrebare(
        cerere.intrebareId, cerere.enunt, cerere.raspunsCorect,
        cerere.punctajMaxim, cerere.ordine);
}

bool EvaluareService::stergeIntrebare(int actorId,
                                      int chestionarId,
                                      int intrebareId) {
    const auto evaluare = obtineEvaluareExistenta(chestionarId);
    verificaAdministrareEvaluare(actorId, evaluare);
    const auto intrebari = evaluari.listeazaIntrebari(chestionarId);
    if (std::none_of(intrebari.begin(), intrebari.end(), [&](const auto& intrebare) {
            return intrebare.id == intrebareId;
        })) {
        throw ExceptieEdu("Intrebarea nu apartine chestionarului specificat.");
    }
    return evaluari.stergeIntrebare(intrebareId);
}

std::vector<IntrebareChestionarInregistrare> EvaluareService::listeazaIntrebari(
    int chestionarId) {
    return evaluari.listeazaIntrebari(chestionarId);
}
std::vector<IntrebareChestionarInregistrare> EvaluareService::listeazaIntrebari(int actorId,int id){const auto e=obtineEvaluareExistenta(id);if(!inscrieri)throw ExceptieEdu("Serviciul de inscrieri nu este disponibil.");inscrieri->verificaAccesStudentLaCurs(actorId,e.cursId);return evaluari.listeazaIntrebari(id);}

int EvaluareService::pornesteIncercare(int studentId, int evaluareId) {
    reguli.verificaStudent(studentId);
    const auto evaluare=obtineEvaluareExistenta(evaluareId);
    if(inscrieri)inscrieri->verificaAccesStudentLaCurs(studentId,evaluare.cursId);
    return evaluari.adaugaIncercare(evaluareId, studentId);
}

void EvaluareService::salveazaRaspuns(const CerereRaspuns& cerere) {
    const auto incercare = obtineIncercareStudent(cerere.actorId, cerere.incercareId);
    if (incercare.finalizataLa) {
        throw ExceptieEdu("Incercarea este deja finalizata.");
    }
    const auto intrebare = evaluari.cautaIntrebareDupaId(cerere.intrebareId);
    if (!intrebare || intrebare->chestionarId != incercare.evaluareId) {
        throw ExceptieEdu("Intrebarea nu apartine evaluarii incercarii.");
    }
    const double punctaj = normalizeazaRaspuns(cerere.raspuns) ==
                                   normalizeazaRaspuns(intrebare->raspunsCorect)
        ? intrebare->punctajMaxim : 0.0;
    evaluari.salveazaRaspuns(
        cerere.incercareId, cerere.intrebareId,
        cerere.raspuns, punctaj);
}

IncercareEvaluareInregistrare EvaluareService::finalizeazaIncercare(
    int studentId, int incercareId) {
    const auto incercare = obtineIncercareStudent(studentId, incercareId);
    if (incercare.finalizataLa) {
        throw ExceptieEdu("Incercarea este deja finalizata.");
    }
    const auto intrebari = evaluari.listeazaIntrebari(incercare.evaluareId);
    double punctajMaxim = 0.0;
    for (const auto& intrebare : intrebari) punctajMaxim += intrebare.punctajMaxim;
    if (!std::isfinite(punctajMaxim) || punctajMaxim <= 0.0) {
        throw ExceptieEdu("Punctajul maxim al evaluarii trebuie sa fie pozitiv.");
    }
    double scorBrut = 0.0;
    for (const auto& raspuns : evaluari.listeazaRaspunsuri(incercareId)) {
        scorBrut += raspuns.punctajObtinut;
    }
    double notaFinala = std::round((scorBrut / punctajMaxim * 10.0) * 100.0) / 100.0;
    notaFinala = std::max(0.0, std::min(10.0, notaFinala));
    if (!evaluari.finalizeazaIncercare(incercareId, scorBrut, notaFinala)) {
        throw ExceptieEdu("Incercarea este deja finalizata.");
    }
    return *evaluari.cautaIncercareDupaId(incercareId);
}

std::optional<IncercareEvaluareInregistrare> EvaluareService::obtineIncercare(
    int studentId,
    int incercareId) {
    return obtineIncercareStudent(studentId, incercareId);
}
