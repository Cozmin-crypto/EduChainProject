#include "UtilizatorBaza.h"

#include "ExceptieEdu.h"

void UtilizatorBaza::actualizeazaParola(const std::string& parolaNoua) {
    if (parolaNoua.empty()) {
        throw ExceptieEdu("Parola nu poate fi goala.");
    }

    parola = parolaNoua;
}

bool UtilizatorBaza::operator==(const UtilizatorBaza& altul) const {
    return email == altul.email;
}
