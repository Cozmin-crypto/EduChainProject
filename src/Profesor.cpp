#include "Profesor.h"

void Profesor::autentificare() {
    // Autentificarea necesita validarea credentialelor prin server.
}

std::string Profesor::obtineRol() const {
    return "Profesor";
}

void Profesor::creeazaContinut() {
    // Crearea continutului va fi delegata modulului de cursuri si serverului.
}

void Profesor::noteazaStudent() {
    // Persistenta notelor va fi realizata prin server si baza de date.
}
