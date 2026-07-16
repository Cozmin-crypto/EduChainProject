#pragma once

#include "Personal.h"

class Administrator : public Personal {
private:
    int nivelAcces{};

public:
    void autentificare() override;
    std::string obtineRol() const override;
    void faceCopieSigurantaBazaDate();
};
