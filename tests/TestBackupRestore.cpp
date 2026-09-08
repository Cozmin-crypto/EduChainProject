#include "ConectorBazaDate.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void verifica(bool conditie, const char* mesaj) {
    if (!conditie) throw std::runtime_error(mesaj);
}
}

int main() {
    const auto baza = std::filesystem::temp_directory_path() / "educhain-backup-test.db";
    const auto backup = std::filesystem::path(baza.string() + ".backup");
    const auto corupta = std::filesystem::path(baza.string() + ".corrupt");
    std::error_code eroare;
    std::filesystem::remove(baza, eroare);
    std::filesystem::remove(backup, eroare);
    std::filesystem::remove(corupta, eroare);
    try {
        {
            ConectorBazaDate conector;
            conector.deschideConexiune(baza.string());
            conector.executaInterogareParametrizata(
                "INSERT INTO utilizatori(nume,prenume,email,parola,rol) VALUES(?,?,?,?,?);",
                {"Ionescu", "Mara", "mara.backup@example.ro", "legacy", "student"});
            conector.creeazaBackup();
            conector.inchideConexiune();
        }
        verifica(std::filesystem::exists(backup), "backup-ul nu a fost creat");
        {
            std::ofstream fisier(baza, std::ios::binary | std::ios::trunc);
            fisier << "baza corupta";
        }
        {
            ConectorBazaDate conector;
            conector.deschideConexiune(baza.string());
            const auto randuri = conector.executaSelectParametrizat(
                "SELECT nume,prenume FROM utilizatori WHERE email=?;",
                {"mara.backup@example.ro"});
            verifica(randuri.size() == 1 && randuri[0][0] == "Ionescu" &&
                         randuri[0][1] == "Mara",
                     "restore-ul nu a pastrat datele");
            verifica(conector.executaSelect("PRAGMA integrity_check;")[0][0] == "ok",
                     "baza restaurata nu este integra");
            conector.inchideConexiune();
        }
        verifica(std::filesystem::exists(corupta), "baza corupta nu a fost conservata");
        std::filesystem::remove(baza, eroare);
        std::filesystem::remove(backup, eroare);
        std::filesystem::remove(corupta, eroare);
        std::cout << "TestBackupRestore: PASS\n";
        return 0;
    } catch (const std::exception& exceptie) {
        std::cerr << "TestBackupRestore: FAIL: " << exceptie.what() << '\n';
        return 1;
    }
}
