#pragma once

#include "LectieRepository.h"
#include "ReguliAccesService.h"

#include <optional>
#include <string>
#include <vector>

class CursRepository;
class UtilizatorRepository;
class InscriereService;

struct CerereSalvareLectie {
    int actorId{};
    int cursId{};
    std::string nume;
    std::string tip;
    std::string continut;
    long long dimensiuneOcteti{};
    std::optional<long long> numarCuvinte;
    std::optional<long long> durata;
    std::optional<std::string> codec;
};

struct CerereActualizareLectie : CerereSalvareLectie {
    int lectieId{};
};

class LectieService {
private:
    LectieRepository& lectii;
    CursRepository& cursuri;
    ReguliAccesService reguli;
    InscriereService* inscrieri{};

    CursInregistrare obtineCursExistent(int cursId);
    LectieInregistrare obtineLectieExistenta(int lectieId);
    void verificaAccesCitireLectie(int actorId, int cursId);

public:
    LectieService(LectieRepository& lectii,
                  CursRepository& cursuri,
                  UtilizatorRepository& utilizatori);
    LectieService(LectieRepository&,CursRepository&,UtilizatorRepository&,InscriereService&);

    std::vector<LectieInregistrare> listeazaDupaCurs(int cursId);
    std::vector<LectieInregistrare> listeazaDupaCurs(int actorId,int cursId);
    std::optional<LectieInregistrare> obtineLectie(int lectieId);
    std::optional<LectieInregistrare> obtineLectie(int actorId,int lectieId);
    int creeazaLectie(const CerereSalvareLectie& cerere);
    bool actualizeazaLectie(const CerereActualizareLectie& cerere);
    bool stergeLectie(int actorId, int lectieId);
};
