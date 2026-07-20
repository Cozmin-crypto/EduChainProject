#include "CursService.h"

#include "ExceptieEdu.h"
#include "UtilizatorRepository.h"
#include "InscriereService.h"

CursService::CursService(CursRepository& cursuri,
                         UtilizatorRepository& utilizatori)
    : cursuri(cursuri), utilizatori(utilizatori), reguli(utilizatori) {
}
CursService::CursService(CursRepository& c,UtilizatorRepository& u,InscriereService& i):cursuri(c),utilizatori(u),reguli(u),inscrieri(&i){}

CursInregistrare CursService::obtineCursExistent(int cursId) {
    const auto curs = cursuri.cautaDupaId(cursId);
    if (!curs.has_value()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
    return *curs;
}
std::vector<CursInregistrare> CursService::listeazaCursuri(int actorId){const auto actor=reguli.obtineActor(actorId);if(actor.rol=="student"){if(!inscrieri)throw ExceptieEdu("Serviciul de inscrieri nu este disponibil.");return inscrieri->listeazaCursuriInscrise(actorId);}return cursuri.listeazaCursuri();}

std::vector<CursInregistrare> CursService::listeazaCursuri() {
    return cursuri.listeazaCursuri();
}

std::optional<CursInregistrare> CursService::obtineCurs(int cursId) {
    return cursuri.cautaDupaId(cursId);
}
std::optional<CursInregistrare> CursService::obtineCurs(int actorId,int cursId){const auto c=cursuri.cautaDupaId(cursId);if(c){if(!inscrieri)throw ExceptieEdu("Serviciul de inscrieri nu este disponibil.");inscrieri->verificaAccesStudentLaCurs(actorId,cursId);}return c;}

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
