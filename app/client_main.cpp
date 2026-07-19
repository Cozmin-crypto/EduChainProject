#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ProtocolEdu.h"

#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

namespace {

constexpr std::uint16_t portImplicit = 5050;
constexpr const char* hostImplicit = "127.0.0.1";

bool parseazaPort(std::string_view text, std::uint16_t& port) {
    unsigned int valoare{};
    const auto rezultat = std::from_chars(text.data(), text.data() + text.size(), valoare);
    if (rezultat.ec != std::errc{} || rezultat.ptr != text.data() + text.size() ||
        valoare == 0 || valoare > 65535) {
        return false;
    }
    port = static_cast<std::uint16_t>(valoare);
    return true;
}

void afiseazaAjutor() {
    std::cout << "Comenzi disponibile:\n"
              << "  help | ajutor                 Afiseaza acest mesaj\n"
              << "  ping                          Trimite PING\n"
              << "  login | autentificare         Cere emailul si parola\n"
              << "  register | inregistrare       Creeaza un cont de student sau profesor\n"
              << "  list-courses | cursuri        Listeaza cursurile accesibile\n"
              << "  list-enrolled-courses | cursuri-inscrise\n"
              << "                                Listeaza cursurile inscrise ale studentului\n"
              << "  logout | deconectare          Inchide sesiunea si conexiunea curenta\n"
              << "  exit | iesire                 Inchide clientul\n";
}

void afiseazaCursuri(const std::vector<CursPublicEdu>& cursuri) {
    if (cursuri.empty()) {
        std::cout << "Nu exista cursuri disponibile.\n";
        return;
    }
    for (const auto& curs : cursuri) {
        std::cout << "ID: " << curs.id << " | Titlu: " << curs.nume
                  << " | Proprietar: " << curs.proprietarId << '\n';
    }
}

void asiguraConectat(ClientEdu& client, const std::string& host, std::uint16_t port) {
    if (!client.esteConectat()) {
        client.pornesteNod();
        std::cout << "Conectat la " << host << ':' << port << "\n";
    }
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc > 3) {
        std::cerr << "Utilizare: EduChainClient.exe [host] [port]\n";
        return 2;
    }

    const std::string host = argc >= 2 ? argv[1] : hostImplicit;
    if (host.empty()) {
        std::cerr << "Host-ul nu poate fi gol.\n";
        return 2;
    }

    std::uint16_t port = portImplicit;
    if (argc == 3 && !parseazaPort(argv[2], port)) {
        std::cerr << "Port invalid. Foloseste un numar intre 1 si 65535.\n";
        return 2;
    }

    try {
        ClientEdu client(port, host);
        asiguraConectat(client, host, port);
        afiseazaAjutor();

        std::string linie;
        while (std::cout << "> ", std::getline(std::cin, linie)) {
            std::istringstream intrare(linie);
            std::string comanda;
            intrare >> comanda;
            if (comanda.empty()) {
                continue;
            }

            try {
                if (comanda == "help" || comanda == "ajutor") {
                    afiseazaAjutor();
                } else if (comanda == "ping") {
                    asiguraConectat(client, host, port);
                    std::cout << client.trimiteCerere("PING") << '\n';
                } else if (comanda == "login" || comanda == "autentificare") {
                    asiguraConectat(client, host, port);
                    std::string email;
                    std::string parola;
                    std::cout << "Email: ";
                    if (!std::getline(std::cin, email)) {
                        break;
                    }
                    std::cout << "Parola: ";
                    if (!std::getline(std::cin, parola)) {
                        break;
                    }
                    const auto raspuns = client.autentifica(email, parola);
                    if (raspuns.cod == CodRezultatEdu::Succes) {
                        std::cout << "Autentificare reusita\n"
                                  << "Rol: " << client.obtineRolAutentificat() << '\n';
                    } else {
                        std::cout << raspuns.mesajPublic << '\n';
                    }
                } else if (comanda == "register" || comanda == "inregistrare") {
                    asiguraConectat(client, host, port);
                    std::string nume, prenume, email, parola, confirmare, rol, optiuneRol;
                    std::cout << "Alege rolul:\n"
                              << "  1. Student\n"
                              << "  2. Profesor\n"
                              << "Optiune: ";
                    if (!std::getline(std::cin, optiuneRol)) break;
                    if (optiuneRol == "1") {
                        rol = "student";
                    } else if (optiuneRol == "2") {
                        rol = "profesor";
                    } else {
                        std::cout << "Optiune invalida. Introdu 1 pentru student sau 2 pentru profesor.\n";
                        continue;
                    }
                    std::cout << "Nume: "; if (!std::getline(std::cin, nume)) break;
                    std::cout << "Prenume: "; if (!std::getline(std::cin, prenume)) break;
                    std::cout << "Email: "; if (!std::getline(std::cin, email)) break;
                    std::cout << "Parola: "; if (!std::getline(std::cin, parola)) break;
                    std::cout << "Confirma parola: "; if (!std::getline(std::cin, confirmare)) break;
                    if (parola != confirmare) { std::cout << "Parolele nu coincid.\n"; continue; }
                    client.inregistreaza(nume, prenume, email, parola, rol);
                    std::cout << "Cont de " << rol << " creat cu succes.\nTe poti autentifica folosind comanda login.\n";
                } else if (comanda == "list-courses" || comanda == "cursuri") {
                    asiguraConectat(client, host, port);
                    afiseazaCursuri(client.listeazaCursuri());
                } else if (comanda == "list-enrolled-courses" ||
                           comanda == "cursuri-inscrise") {
                    asiguraConectat(client, host, port);
                    afiseazaCursuri(client.listeazaCursuriInscrise());
                } else if (comanda == "logout" || comanda == "deconectare") {
                    if (client.esteConectat()) {
                        client.deconecteaza();
                    }
                    std::cout << "Sesiunea a fost inchisa. Urmatoarea comanda se va reconecta.\n";
                } else if (comanda == "exit" || comanda == "iesire") {
                    if (client.esteConectat()) {
                        client.deconecteaza();
                    }
                    return 0;
                } else {
                    std::cout << "Comanda necunoscuta. Foloseste help.\n";
                }
            } catch (const ExceptieEdu& exceptie) {
                std::cerr << "Eroare: " << exceptie.what() << '\n';
            } catch (const std::exception& exceptie) {
                std::cerr << "Eroare standard: " << exceptie.what() << '\n';
            }
        }

        if (client.esteConectat()) {
            client.deconecteaza();
        }
        return 0;
    } catch (const ExceptieEdu& exceptie) {
        std::cerr << "Nu s-a putut porni clientul: " << exceptie.what() << '\n';
        return 1;
    } catch (const std::exception& exceptie) {
        std::cerr << "Eroare standard: " << exceptie.what() << '\n';
        return 1;
    }
}
