#include "CursService.h"

#include "ExceptieEdu.h"
#include "UtilizatorRepository.h"

CursService::CursService(CursRepository& cursuri,
                         UtilizatorRepository& utilizatori)
    : cursuri(cursuri), utilizatori(utilizatori), reguli(utilizatori) {
}

CursInregistrare CursService::obtineCursExistent(int cursId) {
    const auto curs = cursuri.cautaDupaId(cursId);
    if (!curs.has_value()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
    return *curs;
}

std::vector<CursInregistrare> CursService::listeazaCursuri() {
    return cursuri.listeazaCursuri();
}

std::optional<CursInregistrare> CursService::obtineCurs(int cursId) {
    return cursuri.cautaDupaId(cursId);
}

int CursService::creeazaCurs(const CerereCreareCurs& cerere) {
    const auto actor = reguli.obtineActor(cerere.actorId);
    const auto proprietar = utilizatori.cautaDupaId(cerere.proprietarId);
    if (!proprietar.has_value() || proprietar->rol != "profesor") {
        throw ExceptieEdu("Proprietarul cursului trebuie sa fie un profesor valid.");
    }
    if (actor.rol == "student") {
        throw ExceptieEdu("Studentul nu poate crea cursuri.");
    }
    if (actor.rol == "profesor" && actor.id != cerere.proprietarId) {
        throw ExceptieEdu("Profesorul poate crea doar cursuri proprii.");
    }
    if (actor.rol != "profesor" && actor.rol != "administrator") {
        throw ExceptieEdu("Rolul utilizatorului nu permite crearea cursurilor.");
    }
    if (cerere.parinteId.has_value()) {
        reguli.verificaAdministratorSauProprietar(
            cerere.actorId, obtineCursExistent(*cerere.parinteId));
    }
    return cursuri.adaugaCurs(cerere.nume, cerere.parinteId, cerere.proprietarId);
}

bool CursService::actualizeazaCurs(const CerereActualizareCurs& cerere) {
    const auto curs = obtineCursExistent(cerere.cursId);
    reguli.verificaAdministratorSauProprietar(cerere.actorId, curs);
    if (cerere.parinteId.has_value()) {
        reguli.verificaAdministratorSauProprietar(
            cerere.actorId, obtineCursExistent(*cerere.parinteId));
    }
    return cursuri.actualizeazaCurs(
        curs.id, cerere.nume, cerere.parinteId, curs.proprietarId);
}

bool CursService::stergeCurs(int actorId, int cursId) {
    const auto curs = obtineCursExistent(cursId);
    reguli.verificaAdministratorSauProprietar(actorId, curs);
    return cursuri.stergeCurs(cursId);
}
