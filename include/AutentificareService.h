#pragma once

#include <optional>
#include <string>

class UtilizatorRepository;

enum class StareAutentificare {
    Succes,
    EmailInexistent,
    ParolaIncorecta
};

struct RezultatAutentificare {
    bool succes{};
    StareAutentificare stare{StareAutentificare::EmailInexistent};
    std::optional<int> utilizatorId;
    std::string email;
    std::optional<std::string> rol;
    std::string mesajPublic;
};

class AutentificareService {
private:
    UtilizatorRepository& utilizatori;

public:
    explicit AutentificareService(UtilizatorRepository& utilizatori);
    RezultatAutentificare autentifica(const std::string& email,
                                      const std::string& parola);
    int inregistreazaStudent(const std::string&,const std::string&,const std::string&,const std::string&);
};
