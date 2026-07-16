#pragma once

#include "Evaluare.h"

class ExamenFinal : public Evaluare {
private:
    float pondere{};

public:
    float calculeazaScor() const;
    void genereazaCertificat();
};
