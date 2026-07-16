#pragma once

class Evaluare {
protected:
    int limitaTimp{};
    bool esteObligatorie{};

    Evaluare() = default;

public:
    virtual ~Evaluare() = default;
    virtual float calculeazaScor() const = 0;
};
