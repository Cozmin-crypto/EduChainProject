#pragma once

#include <memory>
#include <vector>

#include "Catalog.h"

class Lectie;

class Curs {
private:
    std::vector<std::shared_ptr<Lectie>> lectii;
    Catalog<std::shared_ptr<Lectie>> catalog;

public:
    void afiseazaInformatii() const;
    void adaugaLectie(const std::shared_ptr<Lectie>& lectie);
    Lectie& operator[](int index);
};
