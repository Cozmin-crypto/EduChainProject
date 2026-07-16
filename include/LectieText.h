#pragma once

#include "Lectie.h"

class LectieText : public Lectie {
private:
    int numarCuvinte{};

public:
    void afiseazaInformatii() const;
    void incarcaContinut();
    void formateazaText();
};
