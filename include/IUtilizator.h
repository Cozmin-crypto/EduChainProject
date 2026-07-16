#pragma once

#include <string>

class IUtilizator {
public:
    virtual ~IUtilizator() = default;
    virtual void autentificare() = 0;
    virtual std::string obtineRol() const = 0;
};
