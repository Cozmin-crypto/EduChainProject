#pragma once

#include <optional>
#include <string>

class ConectorBazaDate;

struct UtilizatorInregistrare {
    int id{};
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
                         const std::string& rol);
    bool actualizeazaUtilizator(int id,
                                const std::string& email,
                                const std::string& parola,
                                const std::string& rol);
    bool stergeUtilizator(int id);
    std::optional<UtilizatorInregistrare> cautaDupaId(int id);
    std::optional<UtilizatorInregistrare> cautaDupaEmail(const std::string& email);
    int inregistreazaUtilizator(const std::string& email,
                               const std::string& parola,
                               const std::string& rol);
    int inregistreazaStudent(const std::string& email, const std::string& parola);
};
