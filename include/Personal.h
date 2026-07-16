#pragma once

#include <chrono>
#include <string>

#include "UtilizatorBaza.h"

class Personal : public UtilizatorBaza {
protected:
    std::string departament;
    std::chrono::system_clock::time_point dataAngajarii;

public:
    void vizualizeazaJurnale() const;
};
