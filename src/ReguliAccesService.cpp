#include "ReguliAccesService.h"

#include "ExceptieEdu.h"

ReguliAccesService::ReguliAccesService(UtilizatorRepository& utilizatori)
    : utilizatori(utilizatori) {
}

UtilizatorInregistrare ReguliAccesService::obtineActor(int actorId) {
    if (actorId <= 0) {
        throw ExceptieEdu("Id-ul actorului trebuie sa fie strict pozitiv.");
    }
    const auto actor = utilizatori.cautaDupaId(actorId);
    if (!actor.has_value()) {
        throw ExceptieEdu("Utilizatorul care solicita operatia nu exista.");
    }
    return *actor;
}

void ReguliAccesService::verificaAdministratorSauProprietar(
    int actorId,
    const CursInregistrare& curs) {
    const auto actor = obtineActor(actorId);
    if (actor.rol == "administrator") {
        return;
    }
    if (actor.rol == "profesor" && curs.proprietarId == actor.id) {
        return;
    }
    throw ExceptieEdu("Utilizatorul nu poate administra acest curs.");
}

void ReguliAccesService::verificaStudent(int actorId) {
    if (obtineActor(actorId).rol != "student") {
        throw ExceptieEdu("Operatia este permisa doar studentilor.");
    }
}
