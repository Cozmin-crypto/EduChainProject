#pragma once

#include "Lectie.h"

class LectieVideo : public Lectie {
private:
    int durata{};
    std::string codec;

public:
    void afiseazaInformatii() const;
    void incarcaContinut();
    void reda();
};
