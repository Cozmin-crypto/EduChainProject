#pragma once

#include <string>

class Lectie {
protected:
    std::string continut;

public:
    virtual ~Lectie() = default;
    virtual void incarcaContinut();
};
