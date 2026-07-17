#include "ConectorBazaDate.h"

#include "ExceptieEdu.h"

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>

#include <sqlite3.h>

namespace {
[[noreturn]] void aruncaEroareSQLite(sqlite3* conexiune,
                                     int codEroare,
                                     const std::string& context) {
    const char* mesaj = conexiune != nullptr
        ? sqlite3_errmsg(conexiune)
        : sqlite3_errstr(codEroare);
    throw ExceptieEdu(context + ": " + mesaj);
}

std::string citesteSchema() {
    std::ifstream fisier("schema.sql", std::ios::binary);
    if (!fisier) {
        throw ExceptieEdu("Fisierul schema.sql nu a putut fi deschis.");
    }

    return {std::istreambuf_iterator<char>(fisier), std::istreambuf_iterator<char>()};
}
}

ConectorBazaDate::~ConectorBazaDate() noexcept {
    if (conexiune != nullptr) {
        sqlite3_close_v2(conexiune);
        conexiune = nullptr;
    }
}

void ConectorBazaDate::deschideConexiune(const std::string& caleBazaDate) {
    if (caleBazaDate.empty()) {
        throw ExceptieEdu("Calea bazei de date nu poate fi goala.");
    }

    if (esteConectat()) {
        throw ExceptieEdu("Conexiunea la baza de date este deja deschisa.");
    }

    std::error_code eroareSistem;
    const bool bazaNoua = !std::filesystem::exists(caleBazaDate, eroareSistem);
    if (eroareSistem) {
        throw ExceptieEdu("Calea bazei de date nu poate fi verificata.");
    }

    sqlite3* conexiuneNoua = nullptr;
    const int rezultat = sqlite3_open_v2(
        caleBazaDate.c_str(),
        &conexiuneNoua,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
        nullptr);

    if (rezultat != SQLITE_OK) {
        aruncaEroareSQLite(conexiuneNoua, rezultat, "Deschiderea bazei de date a esuat");
    }

    conexiune = conexiuneNoua;
    try {
        executaInterogare("PRAGMA foreign_keys = ON;");

        if (bazaNoua) {
            executaInterogare(citesteSchema());
        }
    } catch (...) {
        sqlite3_close_v2(conexiune);
        conexiune = nullptr;
        throw;
    }
}

void ConectorBazaDate::inchideConexiune() {
    if (!esteConectat()) {
        return;
    }

    const int rezultat = sqlite3_close(conexiune);
    if (rezultat != SQLITE_OK) {
        aruncaEroareSQLite(conexiune, rezultat, "Inchiderea bazei de date a esuat");
    }

    conexiune = nullptr;
}

bool ConectorBazaDate::esteConectat() const {
    return conexiune != nullptr;
}

void ConectorBazaDate::executaInterogare(const std::string& interogare) {
    if (interogare.empty()) {
        throw ExceptieEdu("Interogarea bazei de date nu poate fi goala.");
    }

    if (!esteConectat()) {
        throw ExceptieEdu("Nu exista o conexiune deschisa la baza de date.");
    }

    char* mesajEroare = nullptr;
    const int rezultat = sqlite3_exec(conexiune, interogare.c_str(), nullptr, nullptr, &mesajEroare);
    if (rezultat != SQLITE_OK) {
        const std::string mesaj = mesajEroare != nullptr
            ? mesajEroare
            : sqlite3_errmsg(conexiune);
        sqlite3_free(mesajEroare);
        throw ExceptieEdu("Executarea interogarii a esuat: " + mesaj);
    }
}

std::vector<std::vector<std::string>> ConectorBazaDate::executaSelect(
    const std::string& interogare) {
    if (interogare.empty()) {
        throw ExceptieEdu("Interogarea SELECT nu poate fi goala.");
    }

    if (!esteConectat()) {
        throw ExceptieEdu("Nu exista o conexiune deschisa la baza de date.");
    }

    sqlite3_stmt* instructiuneBruta = nullptr;
    const char* restInterogare = nullptr;
    const int pregatire = sqlite3_prepare_v2(
        conexiune,
        interogare.c_str(),
        -1,
        &instructiuneBruta,
        &restInterogare);
    if (pregatire != SQLITE_OK) {
        aruncaEroareSQLite(conexiune, pregatire, "Pregatirea interogarii SELECT a esuat");
    }

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> instructiune(
        instructiuneBruta,
        sqlite3_finalize);

    while (restInterogare != nullptr && *restInterogare != '\0' &&
           std::isspace(static_cast<unsigned char>(*restInterogare))) {
        ++restInterogare;
    }
    if (restInterogare != nullptr && *restInterogare != '\0') {
        throw ExceptieEdu("Interogarea SELECT trebuie sa contina o singura instructiune.");
    }

    const int numarColoane = sqlite3_column_count(instructiune.get());
    if (numarColoane == 0) {
        throw ExceptieEdu("Interogarea nu returneaza rezultate SELECT.");
    }

    std::vector<std::vector<std::string>> rezultate;
    int pas = SQLITE_ROW;
    while ((pas = sqlite3_step(instructiune.get())) == SQLITE_ROW) {
        std::vector<std::string> rand;
        rand.reserve(static_cast<std::size_t>(numarColoane));

        for (int coloana = 0; coloana < numarColoane; ++coloana) {
            if (sqlite3_column_type(instructiune.get(), coloana) == SQLITE_NULL) {
                rand.emplace_back();
                continue;
            }

            const auto* valoare = sqlite3_column_text(instructiune.get(), coloana);
            const int lungime = sqlite3_column_bytes(instructiune.get(), coloana);
            rand.emplace_back(reinterpret_cast<const char*>(valoare),
                              static_cast<std::size_t>(lungime));
        }

        rezultate.push_back(std::move(rand));
    }

    if (pas != SQLITE_DONE) {
        aruncaEroareSQLite(conexiune, pas, "Executarea interogarii SELECT a esuat");
    }

    return rezultate;
}
