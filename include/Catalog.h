#pragma once

#include <algorithm>
#include <utility>
#include <vector>

template <typename T>
class Catalog {
private:
    std::vector<T> elemente;

public:
    void adaugaElement(T element) {
        elemente.push_back(std::move(element));
    }

    void sorteazaElemente() {
        std::sort(elemente.begin(), elemente.end());
    }
};
