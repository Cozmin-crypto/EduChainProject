#pragma once

#include "LectieRepository.h"
#include "ReguliAccesService.h"

#include <optional>
#include <string>
#include <vector>

class CursRepository;
class UtilizatorRepository;

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

    CursInregistrare obtineCursExistent(int cursId);
    LectieInregistrare obtineLectieExistenta(int lectieId);

public:
    LectieService(LectieRepository& lectii,
                  CursRepository& cursuri,
                  UtilizatorRepository& utilizatori);

    std::vector<LectieInregistrare> listeazaDupaCurs(int cursId);
    std::optional<LectieInregistrare> obtineLectie(int lectieId);
    int creeazaLectie(const CerereSalvareLectie& cerere);
    bool actualizeazaLectie(const CerereActualizareLectie& cerere);
    bool stergeLectie(int actorId, int lectieId);
};
