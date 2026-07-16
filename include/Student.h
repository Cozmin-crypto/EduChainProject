#pragma once

#include <memory>
#include <vector>

#include "UtilizatorBaza.h"

class Curs;
template <typename T>
class Catalog;

class Student : public UtilizatorBaza {
private:
    std::vector<int> note;
    std::shared_ptr<Catalog<Curs>> catalog;

public:
    void autentificare() override;
    std::string obtineRol() const override;
    void trimiteEvaluare(int idEvaluare);
    bool operator==(const Student& altul) const;
};
