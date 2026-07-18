#pragma once

#include <optional>
#include <string>
#include <vector>

class ConectorBazaDate;

struct LectieInregistrare {
    int id{};
    int cursId{};
    int proprietarId{};
    std::string nume;
    std::string tip;
    std::string continut;
    long long dimensiuneOcteti{};
    std::string creatLa;
    std::string actualizatLa;
    std::optional<long long> numarCuvinte;
    std::optional<long long> durata;
    std::optional<std::string> codec;
};

class LectieRepository {
private:
    ConectorBazaDate& conector;

    void valideazaCursExistent(int cursId);
    void valideazaProfesorExistent(int profesorId);

public:
    explicit LectieRepository(ConectorBazaDate& conector);

    int adaugaLectie(int cursId,
                     int proprietarId,
                     const std::string& nume,
                     const std::string& tip,
                     const std::string& continut,
                     long long dimensiuneOcteti,
                     std::optional<long long> numarCuvinte = std::nullopt,
                     std::optional<long long> durata = std::nullopt,
                     std::optional<std::string> codec = std::nullopt);
    bool actualizeazaLectie(int id,
                            int cursId,
                            int proprietarId,
                            const std::string& nume,
                            const std::string& tip,
                            const std::string& continut,
                            long long dimensiuneOcteti,
                            std::optional<long long> numarCuvinte = std::nullopt,
                            std::optional<long long> durata = std::nullopt,
                            std::optional<std::string> codec = std::nullopt);
    bool stergeLectie(int id);
    std::optional<LectieInregistrare> cautaDupaId(int id);
    std::vector<LectieInregistrare> listeazaDupaCurs(int cursId);
    std::vector<LectieInregistrare> listeazaLectii();
};
