#include "ConectorBazaDate.h"

#include "ExceptieEdu.h"

void ConectorBazaDate::executaInterogare(const std::string& interogare) {
    if (interogare.empty()) {
        throw ExceptieEdu("Interogarea bazei de date nu poate fi goala.");
    }

    // TODO: conectarea si executia interogarilor necesita modulul bazei de date.
}
