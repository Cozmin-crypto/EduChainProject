#pragma once

#include "EvaluareRepository.h"
#include "ReguliAccesService.h"

#include <optional>
#include <string>
#include <vector>

class CursRepository;
class UtilizatorRepository;

struct CerereSalvareEvaluare {
    int actorId{};
    int cursId{};
    std::string nume;
    std::string tip;
    long long limitaTimp{};
    bool esteObligatorie{};
    std::optional<long long> numarIntrebari;
    std::optional<double> pondere;
};

struct CerereActualizareEvaluare : CerereSalvareEvaluare {
    int evaluareId{};
};

struct CerereIntrebare {
    int actorId{};
    int chestionarId{};
    std::string enunt;
    double punctajMaxim{};
    long long ordine{};
};

struct CerereActualizareIntrebare : CerereIntrebare {
    int intrebareId{};
};

struct CerereRaspuns {
    int actorId{};
    int incercareId{};
    int intrebareId{};
    std::string raspuns;
    double punctajObtinut{};
};

class EvaluareService {
private:
    EvaluareRepository& evaluari;
    CursRepository& cursuri;
    ReguliAccesService reguli;

    CursInregistrare obtineCursExistent(int cursId);
    EvaluareInregistrare obtineEvaluareExistenta(int evaluareId);
    IncercareEvaluareInregistrare obtineIncercareStudent(int actorId,
                                                         int incercareId);
    void verificaAdministrareEvaluare(int actorId,
                                      const EvaluareInregistrare& evaluare);

public:
    EvaluareService(EvaluareRepository& evaluari,
                    CursRepository& cursuri,
                    UtilizatorRepository& utilizatori);

    std::vector<EvaluareInregistrare> listeazaDupaCurs(int cursId);
    std::optional<EvaluareInregistrare> obtineEvaluare(int evaluareId);
    int creeazaEvaluare(const CerereSalvareEvaluare& cerere);
    bool actualizeazaEvaluare(const CerereActualizareEvaluare& cerere);
    bool stergeEvaluare(int actorId, int evaluareId);

    int adaugaIntrebare(const CerereIntrebare& cerere);
    bool actualizeazaIntrebare(const CerereActualizareIntrebare& cerere);
    bool stergeIntrebare(int actorId, int chestionarId, int intrebareId);
    std::vector<IntrebareChestionarInregistrare> listeazaIntrebari(int chestionarId);

    int pornesteIncercare(int studentId, int evaluareId);
    void salveazaRaspuns(const CerereRaspuns& cerere);
    bool finalizeazaIncercare(int studentId,
                             int incercareId,
                             double scorBrut,
                             double notaFinala);
    std::optional<IncercareEvaluareInregistrare> obtineIncercare(int studentId,
                                                                 int incercareId);
};
