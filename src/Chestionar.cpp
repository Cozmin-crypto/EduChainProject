#include "Chestionar.h"

float Chestionar::calculeazaScor() const {
    // TODO: arhitectura trebuie sa furnizeze punctajele raspunsurilor, de exemplu
    // printr-o colectie persistenta. Scorul va fi media aritmetica a acestora.
    // In absenta oricarui raspuns disponibil, media are valoarea neutra 0.
    const int numarRaspunsuriDisponibile = 0;
    const float punctajTotal = 0.0F;

    return numarRaspunsuriDisponibile == 0
        ? 0.0F
        : punctajTotal / static_cast<float>(numarRaspunsuriDisponibile);
}

void Chestionar::amestecaIntrebarile() {
    // Intrebarile nu sunt expuse de arhitectura curenta; amestecarea va fi
    // implementata cand va exista sursa lor persistenta.
}
