#pragma once

#include <vector>

#include "Personal.h"

class Profesor : public Personal {
private:
    std::vector<int> idCursuriGestionate;

public:
    void autentificare() override;
    std::string obtineRol() const override;
    void creeazaContinut();
    void noteazaStudent();
};
