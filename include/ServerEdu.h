#pragma once

#include "ManagerSocket.h"

class ServerEdu : public ManagerSocket {
private:
    bool ruleaza{};

public:
    void pornesteNod() override;
    void opresteNod() override;
    void proceseazaCerere();
};
