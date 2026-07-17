#include "ExamenFinal.h"

#include <iostream>

float ExamenFinal::calculeazaScor() const {
    // Ponderea este tratata ca fractie din nota bruta (0-1).
    // TODO: punctajul brut trebuie furnizat de rezultatele persistente ale examenului.
    const float scorBrut = 0.0F;

    return scorBrut * pondere;
}

void ExamenFinal::genereazaCertificat() {
    // Documentatia nu defineste pragul; folosim nota minima 5 pe scara 0-10.
    constexpr float pragPromovare = 5.0F;

    if (calculeazaScor() >= pragPromovare) {
        std::cout << "Certificat EduChain: examen final promovat.\n";
    }
}
