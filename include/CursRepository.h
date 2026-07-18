#pragma once

#include <optional>
#include <string>
#include <vector>

class ConectorBazaDate;

struct CursInregistrare {
    int id{};
    std::string nume;
    std::optional<int> parinteId;
    int proprietarId{};
    std::string creatLa;
    std::string actualizatLa;
};

class CursRepository {
private:
    ConectorBazaDate& conector;

    void valideazaProfesorExistent(int profesorId);

public:
    explicit CursRepository(ConectorBazaDate& conector);

    int adaugaCurs(const std::string& nume,
                   std::optional<int> parinteId,
                   int proprietarId);
    bool actualizeazaCurs(int id,
                         const std::string& nume,
                         std::optional<int> parinteId,
                         int proprietarId);
    bool stergeCurs(int id);
    std::optional<CursInregistrare> cautaDupaId(int id);
    std::vector<CursInregistrare> listeazaCursuri();
    std::vector<CursInregistrare> listeazaDupaProfesor(int profesorId);
};
