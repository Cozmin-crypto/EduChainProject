#pragma once

#include <string>
#include <vector>

struct sqlite3;

class ConectorBazaDate {
private:
    sqlite3* conexiune{};

public:
    ConectorBazaDate() = default;
    ~ConectorBazaDate() noexcept;

    ConectorBazaDate(const ConectorBazaDate&) = delete;
    ConectorBazaDate& operator=(const ConectorBazaDate&) = delete;

    void deschideConexiune(const std::string& caleBazaDate = "EduChain.db");
    void inchideConexiune();
    bool esteConectat() const;
    void executaInterogare(const std::string& interogare);
    std::vector<std::vector<std::string>> executaSelect(const std::string& interogare);
};
