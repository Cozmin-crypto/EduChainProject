#include "Jurnalizare.h"

#include "ExceptieEdu.h"

#include <fstream>
#include <iostream>
#include <vector>

namespace {
std::vector<std::string> actiuni;
constexpr const char* fisierJurnalImplicit = "jurnal_educhain.txt";
}

void Jurnalizare::inregistreazaActiune(const std::string& mesaj) {
    if (mesaj.empty()) {
        throw ExceptieEdu("Mesajul jurnalului nu poate fi gol.");
    }

    actiuni.push_back(mesaj);
    std::clog << mesaj << '\n';
}

void Jurnalizare::exportaInFisier() {
    std::ofstream fisier(fisierJurnalImplicit);
    if (!fisier) {
        throw ExceptieEdu("Jurnalul nu a putut fi exportat.");
    }

    for (const auto& actiune : actiuni) {
        fisier << actiune << '\n';
    }
}
