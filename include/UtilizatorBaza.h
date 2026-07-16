#pragma once

#include <string>

#include "IUtilizator.h"

class UtilizatorBaza : public IUtilizator {
protected:
    int id{};
    std::string email;
    std::string parola;

public:
    void actualizeazaParola(const std::string& parolaNoua);
    bool operator==(const UtilizatorBaza& altul) const;
};
