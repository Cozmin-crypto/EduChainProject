#pragma once

#include "CursRepository.h"
#include "ReguliAccesService.h"

#include <optional>
#include <string>
#include <vector>

class UtilizatorRepository;
class InscriereService;

struct CerereCreareCurs {
    int actorId{};
    int proprietarId{};
    std::string nume;
    std::optional<int> parinteId;
};

struct CerereActualizareCurs {
    int actorId{};
    int cursId{};
    std::string nume;
    std::optional<int> parinteId;
};

class CursService {
private:
    CursRepository& cursuri;
    UtilizatorRepository& utilizatori;
    ReguliAccesService reguli;
    InscriereService* inscrieri{};

    CursInregistrare obtineCursExistent(int cursId);

public:
    CursService(CursRepository& cursuri, UtilizatorRepository& utilizatori);
    CursService(CursRepository&, UtilizatorRepository&, InscriereService&);

    std::vector<CursInregistrare> listeazaCursuri();
    std::vector<CursInregistrare> listeazaCursuri(int actorId);
    std::optional<CursInregistrare> obtineCurs(int cursId);
    std::optional<CursInregistrare> obtineCurs(int actorId,int cursId);
    int creeazaCurs(const CerereCreareCurs& cerere);
    bool actualizeazaCurs(const CerereActualizareCurs& cerere);
    bool stergeCurs(int actorId, int cursId);
};
