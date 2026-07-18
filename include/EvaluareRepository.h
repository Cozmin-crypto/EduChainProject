#pragma once

#include <optional>
#include <string>
#include <vector>

class ConectorBazaDate;

struct EvaluareInregistrare {
    int id{};
    int cursId{};
    int profesorId{};
    std::string nume;
    std::string tip;
    long long limitaTimp{};
    bool esteObligatorie{};
    std::string creatLa;
    std::optional<long long> numarIntrebari;
    std::optional<double> pondere;
};

struct IntrebareChestionarInregistrare {
    int id{};
    int chestionarId{};
    std::string enunt;
    double punctajMaxim{};
    long long ordine{};
};

struct IncercareEvaluareInregistrare {
    int id{};
    int evaluareId{};
    int studentId{};
    std::string inceputaLa;
    std::optional<std::string> finalizataLa;
    double scorBrut{};
    double notaFinala{};
};

struct RaspunsChestionarInregistrare {
    int incercareId{};
    int intrebareId{};
    std::string raspuns;
    double punctajObtinut{};
};

class EvaluareRepository {
private:
    ConectorBazaDate& conector;

    void valideazaCursExistent(int cursId);
    void valideazaProfesorExistent(int profesorId);
    void valideazaStudentExistent(int studentId);
    void valideazaChestionarExistent(int chestionarId);

public:
    explicit EvaluareRepository(ConectorBazaDate& conector);

    int adaugaEvaluare(int cursId,
                       int profesorId,
                       const std::string& nume,
                       const std::string& tip,
                       long long limitaTimp,
                       bool esteObligatorie,
                       std::optional<long long> numarIntrebari = std::nullopt,
                       std::optional<double> pondere = std::nullopt);
    bool actualizeazaEvaluare(int id,
                              int cursId,
                              int profesorId,
                              const std::string& nume,
                              const std::string& tip,
                              long long limitaTimp,
                              bool esteObligatorie,
                              std::optional<long long> numarIntrebari = std::nullopt,
                              std::optional<double> pondere = std::nullopt);
    bool stergeEvaluare(int id);
    std::optional<EvaluareInregistrare> cautaDupaId(int id);
    std::vector<EvaluareInregistrare> listeazaDupaCurs(int cursId);

    int adaugaIntrebare(int chestionarId,
                        const std::string& enunt,
                        double punctajMaxim,
                        long long ordine);
    bool stergeIntrebare(int intrebareId);
    std::vector<IntrebareChestionarInregistrare> listeazaIntrebari(int chestionarId);

    int adaugaIncercare(int evaluareId, int studentId);
    bool finalizeazaIncercare(int incercareId, double scorBrut, double notaFinala);
    std::optional<IncercareEvaluareInregistrare> cautaIncercareDupaId(int incercareId);
    std::vector<IncercareEvaluareInregistrare> listeazaIncercari(int evaluareId);

    void salveazaRaspuns(int incercareId,
                         int intrebareId,
                         const std::string& raspuns,
                         double punctajObtinut);
    std::vector<RaspunsChestionarInregistrare> listeazaRaspunsuri(int incercareId);
};
