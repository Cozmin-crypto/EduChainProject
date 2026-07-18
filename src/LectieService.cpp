#include "LectieService.h"

#include "CursRepository.h"
#include "ExceptieEdu.h"

LectieService::LectieService(LectieRepository& lectii,
                             CursRepository& cursuri,
                             UtilizatorRepository& utilizatori)
    : lectii(lectii), cursuri(cursuri), reguli(utilizatori) {
}

CursInregistrare LectieService::obtineCursExistent(int cursId) {
    const auto curs = cursuri.cautaDupaId(cursId);
    if (!curs.has_value()) {
        throw ExceptieEdu("Cursul specificat nu exista.");
    }
    return *curs;
}

LectieInregistrare LectieService::obtineLectieExistenta(int lectieId) {
    const auto lectie = lectii.cautaDupaId(lectieId);
    if (!lectie.has_value()) {
        throw ExceptieEdu("Lectia specificata nu exista.");
    }
    return *lectie;
}

std::vector<LectieInregistrare> LectieService::listeazaDupaCurs(int cursId) {
    return lectii.listeazaDupaCurs(cursId);
}

std::optional<LectieInregistrare> LectieService::obtineLectie(int lectieId) {
    return lectii.cautaDupaId(lectieId);
}

int LectieService::creeazaLectie(const CerereSalvareLectie& cerere) {
    const auto curs = obtineCursExistent(cerere.cursId);
    reguli.verificaAdministratorSauProprietar(cerere.actorId, curs);
    return lectii.adaugaLectie(
        curs.id, curs.proprietarId, cerere.nume, cerere.tip, cerere.continut,
        cerere.dimensiuneOcteti, cerere.numarCuvinte, cerere.durata, cerere.codec);
}

bool LectieService::actualizeazaLectie(const CerereActualizareLectie& cerere) {
    const auto lectie = obtineLectieExistenta(cerere.lectieId);
    reguli.verificaAdministratorSauProprietar(
        cerere.actorId, obtineCursExistent(lectie.cursId));
    const auto cursDestinatie = obtineCursExistent(cerere.cursId);
    reguli.verificaAdministratorSauProprietar(cerere.actorId, cursDestinatie);
    return lectii.actualizeazaLectie(
        lectie.id, cursDestinatie.id, cursDestinatie.proprietarId,
        cerere.nume, cerere.tip, cerere.continut, cerere.dimensiuneOcteti,
        cerere.numarCuvinte, cerere.durata, cerere.codec);
}

bool LectieService::stergeLectie(int actorId, int lectieId) {
    const auto lectie = obtineLectieExistenta(lectieId);
    reguli.verificaAdministratorSauProprietar(
        actorId, obtineCursExistent(lectie.cursId));
    return lectii.stergeLectie(lectieId);
}
