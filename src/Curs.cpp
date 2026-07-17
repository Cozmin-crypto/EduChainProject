#include "Curs.h"

#include "ExceptieEdu.h"
#include "Lectie.h"

void Curs::afiseazaInformatii() const {
    // Afisarea detaliilor cursului va fi realizata de interfata Qt.
}

void Curs::adaugaLectie(const std::shared_ptr<Lectie>& lectie) {
    if (!lectie) {
        throw ExceptieEdu("Lectia nu poate fi nula.");
    }

    lectii.push_back(lectie);
    catalog.adaugaElement(lectie);
}

Lectie& Curs::operator[](int index) {
    if (index < 0 || static_cast<std::size_t>(index) >= lectii.size()) {
        throw ExceptieEdu("Indexul lectiei este invalid.");
    }

    return *lectii[static_cast<std::size_t>(index)];
}
