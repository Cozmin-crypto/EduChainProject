#include "ConectorBazaDate.h"

#include "ExceptieEdu.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
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

bool verificaIntegritateFisier(const std::string& cale) {
    sqlite3* baza = nullptr;
    if (sqlite3_open_v2(cale.c_str(), &baza, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) {
        if (baza != nullptr) sqlite3_close_v2(baza);
        return false;
    }
    sqlite3_stmt* instructiune = nullptr;
    bool valida = false;
    if (sqlite3_prepare_v2(baza, "PRAGMA integrity_check;", -1, &instructiune, nullptr) ==
        SQLITE_OK && sqlite3_step(instructiune) == SQLITE_ROW) {
        const auto* text = sqlite3_column_text(instructiune, 0);
        valida = text != nullptr && std::string(reinterpret_cast<const char*>(text)) == "ok";
    }
    if (instructiune != nullptr) sqlite3_finalize(instructiune);
    sqlite3_close_v2(baza);
    return valida;
}

void copiazaCuBackupApi(sqlite3* sursa, sqlite3* destinatie) {
    sqlite3_backup* backup = sqlite3_backup_init(destinatie, "main", sursa, "main");
    if (backup == nullptr) {
        aruncaEroareSQLite(destinatie, sqlite3_errcode(destinatie),
                          "Initializarea backup-ului SQLite a esuat");
    }
    const int pas = sqlite3_backup_step(backup, -1);
    const int finalizare = sqlite3_backup_finish(backup);
    if (pas != SQLITE_DONE || finalizare != SQLITE_OK) {
        aruncaEroareSQLite(destinatie, finalizare != SQLITE_OK ? finalizare : pas,
                          "Backup-ul SQLite a esuat");
    }
}
}

void ConectorBazaDate::aplicaMigrariCompatibilitate() {
    executaInterogare(
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "versiune INTEGER NOT NULL, aplicata_la TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP);"
        "INSERT INTO schema_version(versiune) SELECT 1 "
        "WHERE NOT EXISTS (SELECT 1 FROM schema_version);");
    executaInterogare(
        "CREATE TRIGGER IF NOT EXISTS blocheaza_incercare_duplicata "
        "BEFORE INSERT ON incercari_evaluare "
        "WHEN EXISTS (SELECT 1 FROM incercari_evaluare WHERE evaluare_id = NEW.evaluare_id "
        "AND student_id = NEW.student_id) BEGIN "
        "SELECT RAISE(ABORT, 'EVALUARE_DEJA_SUSTINUTA'); END;");
    const auto coloaneUtilizatori = executaSelect("PRAGMA table_info(utilizatori);");
    const auto areColoana = [&](const std::string& numeColoana) {
        return std::any_of(
            coloaneUtilizatori.begin(), coloaneUtilizatori.end(),
            [&](const std::vector<std::string>& coloana) {
                return coloana.size() > 1 && coloana[1] == numeColoana;
            });
    };
    if (!areColoana("nume") || !areColoana("prenume")) {
        executaInterogare("BEGIN IMMEDIATE;");
        try {
            if (!areColoana("nume")) {
                executaInterogare(
                    "ALTER TABLE utilizatori ADD COLUMN nume TEXT NOT NULL DEFAULT ''; ");
            }
            if (!areColoana("prenume")) {
                executaInterogare(
                    "ALTER TABLE utilizatori ADD COLUMN prenume TEXT NOT NULL DEFAULT ''; ");
            }
            executaInterogare(
                "UPDATE utilizatori SET "
                "nume = CASE WHEN trim(nume) = '' THEN "
                "CASE WHEN rol = 'student' THEN 'Student' ELSE 'Profesor' END "
                "ELSE nume END, "
                "prenume = CASE WHEN trim(prenume) = '' THEN CAST(id AS TEXT) "
                "ELSE prenume END;");
            executaInterogare("COMMIT;");
        } catch (...) {
            try {
                executaInterogare("ROLLBACK;");
            } catch (...) {
            }
            throw;
        }
    }

    const auto tabele = executaSelectParametrizat(
        "SELECT name FROM sqlite_master WHERE type = 'table' AND name = ?;",
        {"intrebari_chestionar"});
    if (tabele.empty()) {
        return;
    }

    const auto cheiExterne = executaSelect("PRAGMA foreign_key_list(intrebari_chestionar);");
    bool relatieVeche = false;
    for (const auto& cheie : cheiExterne) {
        // PRAGMA foreign_key_list: coloana 2 este tabela referita.
        if (cheie.size() > 2 && cheie[2] == "chestionare") {
            relatieVeche = true;
            break;
        }
    }
    if (!relatieVeche) {
        return;
    }

    executaInterogare("PRAGMA foreign_keys = OFF;");
    try {
        executaInterogare(
            "BEGIN IMMEDIATE;"
            "ALTER TABLE raspunsuri_chestionar RENAME TO raspunsuri_chestionar_legacy;"
            "ALTER TABLE intrebari_chestionar RENAME TO intrebari_chestionar_legacy;"
            "CREATE TABLE intrebari_chestionar ("
            "id INTEGER PRIMARY KEY,"
            "chestionar_id INTEGER NOT NULL,"
            "enunt TEXT NOT NULL,"
            "raspuns_corect TEXT NOT NULL CHECK (length(trim(raspuns_corect)) > 0),"
            "punctaj_maxim REAL NOT NULL CHECK (punctaj_maxim >= 0.0),"
            "ordine INTEGER NOT NULL CHECK (ordine >= 0),"
            "FOREIGN KEY (chestionar_id) REFERENCES evaluari(id) ON DELETE CASCADE,"
            "UNIQUE (chestionar_id, ordine));"
            "INSERT INTO intrebari_chestionar "
            "(id, chestionar_id, enunt, raspuns_corect, punctaj_maxim, ordine) "
            "SELECT id, chestionar_id, enunt, raspuns_corect, punctaj_maxim, ordine "
            "FROM intrebari_chestionar_legacy;"
            "CREATE TABLE raspunsuri_chestionar ("
            "incercare_id INTEGER NOT NULL,"
            "intrebare_id INTEGER NOT NULL,"
            "raspuns TEXT NOT NULL,"
            "punctaj_obtinut REAL NOT NULL DEFAULT 0.0 CHECK (punctaj_obtinut >= 0.0),"
            "PRIMARY KEY (incercare_id, intrebare_id),"
            "FOREIGN KEY (incercare_id) REFERENCES incercari_evaluare(id) ON DELETE CASCADE,"
            "FOREIGN KEY (intrebare_id) REFERENCES intrebari_chestionar(id) ON DELETE CASCADE);"
            "INSERT INTO raspunsuri_chestionar "
            "(incercare_id, intrebare_id, raspuns, punctaj_obtinut) "
            "SELECT incercare_id, intrebare_id, raspuns, punctaj_obtinut "
            "FROM raspunsuri_chestionar_legacy;"
            "DROP TABLE raspunsuri_chestionar_legacy;"
            "DROP TABLE intrebari_chestionar_legacy;"
            "COMMIT;");
    } catch (...) {
        try {
            executaInterogare("ROLLBACK;");
        } catch (...) {
        }
        executaInterogare("PRAGMA foreign_keys = ON;");
        throw;
    }
    executaInterogare("PRAGMA foreign_keys = ON;");
    if (!executaSelect("PRAGMA foreign_key_check;").empty()) {
        throw ExceptieEdu("Migrarea bazei de date a produs relatii invalide.");
    }
}

ConectorBazaDate::~ConectorBazaDate() noexcept {
    if (conexiune != nullptr) {
        sqlite3_close_v2(conexiune);
        conexiune = nullptr;
    }
}

void ConectorBazaDate::deschideConexiune(const std::string& caleSolicitata) {
    if (caleSolicitata.empty()) {
        throw ExceptieEdu("Calea bazei de date nu poate fi goala.");
    }

    if (esteConectat()) {
        throw ExceptieEdu("Conexiunea la baza de date este deja deschisa.");
    }

    this->caleBazaDate = std::filesystem::absolute(caleSolicitata).string();
    const std::string caleBackup = this->caleBazaDate + ".backup";
    std::cout << "[DB] Database path: " << this->caleBazaDate << '\n';
    std::error_code eroareSistem;
    bool bazaNoua = !std::filesystem::exists(this->caleBazaDate, eroareSistem);
    if (eroareSistem) {
        throw ExceptieEdu("Calea bazei de date nu poate fi verificata.");
    }

    const bool activaValida = !bazaNoua && verificaIntegritateFisier(this->caleBazaDate);
    std::cout << "[DB] Integrity check: " << (bazaNoua ? "MISSING" : activaValida ? "PASS" : "FAIL") << '\n';
    const bool backupExistent = std::filesystem::exists(caleBackup, eroareSistem);
    const bool backupValid = backupExistent && verificaIntegritateFisier(caleBackup);
    std::cout << "[DB] Backup found: " << (backupExistent ? caleBackup : "NO") << '\n';
    std::cout << "[DB] Backup integrity: " << (backupValid ? "PASS" : backupExistent ? "FAIL" : "N/A") << '\n';
    const bool restaureaza = (!bazaNoua && !activaValida && backupValid) || (bazaNoua && backupValid);
    std::cout << "[DB] Restore required: " << (restaureaza ? "YES" : "NO") << '\n';
    if (!bazaNoua && !activaValida && !backupValid) {
        throw ExceptieEdu("Baza de date activa este corupta si nu exista un backup valid.");
    }
    if (restaureaza) {
        const std::string temporar = this->caleBazaDate + ".restore.tmp";
        std::filesystem::remove(temporar, eroareSistem);
        sqlite3* sursa = nullptr;
        sqlite3* destinatie = nullptr;
        if (sqlite3_open_v2(caleBackup.c_str(), &sursa, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK ||
            sqlite3_open_v2(temporar.c_str(), &destinatie,
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) != SQLITE_OK) {
            if (sursa != nullptr) sqlite3_close_v2(sursa);
            if (destinatie != nullptr) sqlite3_close_v2(destinatie);
            throw ExceptieEdu("Restaurarea backup-ului SQLite nu a putut fi initializata.");
        }
        copiazaCuBackupApi(sursa, destinatie);
        sqlite3_close_v2(sursa);
        sqlite3_close_v2(destinatie);
        if (!bazaNoua) {
            std::filesystem::rename(this->caleBazaDate, this->caleBazaDate + ".corrupt", eroareSistem);
            if (eroareSistem) throw ExceptieEdu("Baza corupta nu a putut fi conservata.");
        }
        std::filesystem::rename(temporar, this->caleBazaDate, eroareSistem);
        if (eroareSistem) throw ExceptieEdu("Backup-ul restaurat nu a putut deveni baza activa.");
        bazaNoua = false;
        std::cout << "[DB] Restore completed\n";
    }

    sqlite3* conexiuneNoua = nullptr;
    const int rezultat = sqlite3_open_v2(
        this->caleBazaDate.c_str(),
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
        } else {
            std::cout << "[DB] Applying migrations...\n";
            aplicaMigrariCompatibilitate();
        }
        if (!executaSelect("PRAGMA foreign_key_check;").empty()) {
            throw ExceptieEdu("Schema bazei de date contine relatii invalide.");
        }
        std::cout << "[DB] Schema validation: PASS\n";
    } catch (...) {
        sqlite3_close_v2(conexiune);
        conexiune = nullptr;
        throw;
    }
}

void ConectorBazaDate::creeazaBackup() {
    if (!esteConectat() || caleBazaDate.empty()) {
        throw ExceptieEdu("Backup-ul necesita o conexiune deschisa.");
    }
    const std::string caleBackup = caleBazaDate + ".backup";
    const std::string temporar = caleBackup + ".tmp";
    std::error_code eroareSistem;
    std::filesystem::remove(temporar, eroareSistem);
    sqlite3* destinatie = nullptr;
    const int rezultat = sqlite3_open_v2(
        temporar.c_str(), &destinatie, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rezultat != SQLITE_OK) {
        aruncaEroareSQLite(destinatie, rezultat, "Deschiderea backup-ului SQLite a esuat");
    }
    try {
        copiazaCuBackupApi(conexiune, destinatie);
        sqlite3_close_v2(destinatie);
        destinatie = nullptr;
        if (!verificaIntegritateFisier(temporar)) {
            throw ExceptieEdu("Backup-ul SQLite nou nu a trecut verificarea de integritate.");
        }
        std::filesystem::remove(caleBackup, eroareSistem);
        eroareSistem.clear();
        std::filesystem::rename(temporar, caleBackup, eroareSistem);
        if (eroareSistem) throw ExceptieEdu("Backup-ul SQLite nu a putut fi publicat atomic.");
        std::cout << "[DB] Backup updated: PASS\n";
    } catch (...) {
        if (destinatie != nullptr) sqlite3_close_v2(destinatie);
        std::filesystem::remove(temporar, eroareSistem);
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
    return executaSelectParametrizat(interogare, {});
}

namespace {
std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> pregatesteInterogare(
    sqlite3* conexiune,
    const std::string& interogare,
    const std::vector<std::string>& parametri) {
    if (interogare.empty()) {
        throw ExceptieEdu("Interogarea parametrizata nu poate fi goala.");
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
        aruncaEroareSQLite(conexiune, pregatire, "Pregatirea interogarii a esuat");
    }

    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)> instructiune(
        instructiuneBruta,
        sqlite3_finalize);

    while (restInterogare != nullptr && *restInterogare != '\0' &&
           std::isspace(static_cast<unsigned char>(*restInterogare))) {
        ++restInterogare;
    }
    if (restInterogare != nullptr && *restInterogare != '\0') {
        throw ExceptieEdu("Interogarea parametrizata trebuie sa contina o singura instructiune.");
    }

    if (sqlite3_bind_parameter_count(instructiune.get()) !=
        static_cast<int>(parametri.size())) {
        throw ExceptieEdu("Numarul parametrilor nu corespunde interogarii.");
    }

    for (std::size_t index = 0; index < parametri.size(); ++index) {
        const int rezultat = sqlite3_bind_text(
            instructiune.get(),
            static_cast<int>(index + 1),
            parametri[index].c_str(),
            static_cast<int>(parametri[index].size()),
            SQLITE_TRANSIENT);
        if (rezultat != SQLITE_OK) {
            aruncaEroareSQLite(conexiune, rezultat, "Legarea parametrilor a esuat");
        }
    }

    return instructiune;
}
}

int ConectorBazaDate::executaInterogareParametrizata(
    const std::string& interogare,
    const std::vector<std::string>& parametri) {
    if (!esteConectat()) {
        throw ExceptieEdu("Nu exista o conexiune deschisa la baza de date.");
    }

    auto instructiune = pregatesteInterogare(conexiune, interogare, parametri);
    const int pas = sqlite3_step(instructiune.get());
    if (pas != SQLITE_DONE) {
        aruncaEroareSQLite(conexiune, pas, "Executarea interogarii parametrizate a esuat");
    }

    return sqlite3_changes(conexiune);
}

std::vector<std::vector<std::string>> ConectorBazaDate::executaSelectParametrizat(
    const std::string& interogare,
    const std::vector<std::string>& parametri) {
    if (!esteConectat()) {
        throw ExceptieEdu("Nu exista o conexiune deschisa la baza de date.");
    }

    auto instructiune = pregatesteInterogare(conexiune, interogare, parametri);
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
