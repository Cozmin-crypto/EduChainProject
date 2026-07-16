#pragma once

#include <string>

class Jurnalizare {
public:
    void inregistreazaActiune(const std::string& mesaj);
    void exportaInFisier();
};
