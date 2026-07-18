#include "EvaluareService.h"

#include "CursRepository.h"
#include "ExceptieEdu.h"

#include <algorithm>

EvaluareService::EvaluareService(EvaluareRepository& evaluari,
                                 CursRepository& cursuri,
                                 UtilizatorRepository& utilizatori)
    : evaluari(evaluari), cursuri(cursuri), reguli(utilizatori) {
}

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
        evaluare.id, cerere.enunt, cerere.punctajMaxim, cerere.ordine);
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
        cerere.intrebareId, cerere.enunt, cerere.punctajMaxim, cerere.ordine);
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

int EvaluareService::pornesteIncercare(int studentId, int evaluareId) {
    reguli.verificaStudent(studentId);
    obtineEvaluareExistenta(evaluareId);
    return evaluari.adaugaIncercare(evaluareId, studentId);
}

void EvaluareService::salveazaRaspuns(const CerereRaspuns& cerere) {
    obtineIncercareStudent(cerere.actorId, cerere.incercareId);
    evaluari.salveazaRaspuns(
        cerere.incercareId, cerere.intrebareId,
        cerere.raspuns, cerere.punctajObtinut);
}

bool EvaluareService::finalizeazaIncercare(int studentId,
                                           int incercareId,
                                           double scorBrut,
                                           double notaFinala) {
    obtineIncercareStudent(studentId, incercareId);
    return evaluari.finalizeazaIncercare(incercareId, scorBrut, notaFinala);
}

std::optional<IncercareEvaluareInregistrare> EvaluareService::obtineIncercare(
    int studentId,
    int incercareId) {
    return obtineIncercareStudent(studentId, incercareId);
}
