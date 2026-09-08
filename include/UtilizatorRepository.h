#pragma once

#include <optional>
#include <string>

class ConectorBazaDate;

struct UtilizatorInregistrare {
    int id{};
    std::string nume;
    std::string prenume;
    std::string email;
    std::string parola;
    std::string rol;
    std::string dataUltimaLogare;
    std::string creatLa;
};

class UtilizatorRepository {
private:
    ConectorBazaDate& conector;

public:
    explicit UtilizatorRepository(ConectorBazaDate& conector);

    int adaugaUtilizator(const std::string& email,
                         const std::string& parola,
                         const std::string& rol,
                         const std::string& nume = {},
                         const std::string& prenume = {});
    bool actualizeazaUtilizator(int id,
                                const std::string& email,
                                const std::string& parola,
                                const std::string& rol);
    bool stergeUtilizator(int id);
    std::optional<UtilizatorInregistrare> cautaDupaId(int id);
    std::optional<UtilizatorInregistrare> cautaDupaEmail(const std::string& email);
    bool actualizeazaParola(int id, const std::string& parolaHash);
    int inregistreazaUtilizator(const std::string& email,
                               const std::string& parola,
                               const std::string& rol,
                               const std::string& nume,
                               const std::string& prenume);
    int inregistreazaStudent(const std::string& email, const std::string& parola);
};
