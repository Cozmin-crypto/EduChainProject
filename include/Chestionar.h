#pragma once

#include "Evaluare.h"

class Chestionar : public Evaluare {
private:
    int numarIntrebari{};

public:
    float calculeazaScor() const;
    void amestecaIntrebarile();
};
