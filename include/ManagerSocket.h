#pragma once

#include <string>

#include "INodRetea.h"

class ManagerSocket : public INodRetea {
protected:
    int idSocket{};
    std::string adresaIp;
    int port{};

public:
    void gestioneazaPachet();
};
