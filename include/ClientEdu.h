#pragma once

#include <string>

#include "ManagerSocket.h"

class ClientEdu : public ManagerSocket {
private:
    std::string stareConexiune;

public:
    void pornesteNod() override;
    void opresteNod() override;
    void trimiteCerere();
};
