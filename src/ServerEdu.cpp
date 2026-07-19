#include "ServerEdu.h"

#include "AutentificareService.h"
#include "CursService.h"
#include "ExceptieEdu.h"

#include <WS2tcpip.h>

#include <algorithm>
#include <limits>
#include <utility>

namespace {
std::string eroareServer(const std::string& context) {
    return context + " (cod Winsock " + std::to_string(WSAGetLastError()) + ").";
}

RaspunsEdu raspuns(std::uint32_t id,
                   CodRezultatEdu cod,
                   std::string mesaj) {
    return {versiuneProtocolEdu, id, cod, std::move(mesaj), {}};
}

void verificaAutentificare(const SesiuneClient& sesiune) {
    if (!sesiune.autentificat) {
        throw ExceptieEdu("AUTENTIFICARE_NECESARA");
    }
}

std::string campObligatoriu(const CerereEdu& cerere, CampEdu camp) {
    const auto valoare = ProtocolEdu::cautaCamp(cerere.campuri, camp);
    if (!valoare.has_value()) {
        throw ExceptieEdu("Lipseste un camp obligatoriu al cererii.");
    }
    return *valoare;
}

int convertesteId(const std::string& valoare, const std::string& denumire) {
    try {
        std::size_t convertite = 0;
        const long long rezultat = std::stoll(valoare, &convertite);
        if (convertite != valoare.size() || rezultat <= 0 ||
            rezultat > std::numeric_limits<int>::max()) {
            throw ExceptieEdu(denumire + " este invalid.");
        }
        return static_cast<int>(rezultat);
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu(denumire + " este invalid.");
    }
}

void verificaCampuri(const CerereEdu& cerere,
                     std::initializer_list<CampEdu> permise) {
    for (const auto& camp : cerere.campuri) {
        const auto iterator = std::find_if(
            permise.begin(), permise.end(), [&](CampEdu permis) {
                return camp.id == static_cast<std::uint16_t>(permis);
            });
        if (iterator == permise.end()) {
            throw ExceptieEdu("Cererea contine un camp nepermis.");
        }
        if (std::count_if(cerere.campuri.begin(), cerere.campuri.end(),
                          [&](const CampProtocolEdu& altul) {
                              return altul.id == camp.id;
                          }) != 1) {
            throw ExceptieEdu("Cererea contine campuri duplicate.");
        }
    }
}

CodRezultatEdu clasificaExceptie(const std::string& mesaj) {
    if (mesaj == "AUTENTIFICARE_NECESARA" ||
        mesaj.find("nu poate administra") != std::string::npos ||
        mesaj.find("Studentul nu poate") != std::string::npos ||
        mesaj.find("Profesorul poate crea doar") != std::string::npos ||
        mesaj.find("Rolul utilizatorului nu permite") != std::string::npos) {
        return CodRezultatEdu::AccesInterzis;
    }
    if (mesaj.find("nu exista") != std::string::npos) {
        return CodRezultatEdu::ResursaInexistenta;
    }
    if (mesaj.find("UNIQUE constraint failed") != std::string::npos) {
        return CodRezultatEdu::Conflict;
    }
    return CodRezultatEdu::ValidareEsuata;
}

std::string mesajPublicPentru(CodRezultatEdu cod) {
    switch (cod) {
    case CodRezultatEdu::AccesInterzis:
        return "Acces interzis.";
    case CodRezultatEdu::ResursaInexistenta:
        return "Resursa solicitata nu exista.";
    case CodRezultatEdu::Conflict:
        return "Operatia intra in conflict cu datele existente.";
    case CodRezultatEdu::ValidareEsuata:
        return "Datele cererii sunt invalide.";
    default:
        return "Cererea nu a putut fi procesata.";
    }
}

CursPublicEdu cursPublic(const CursInregistrare& curs) {
    return {curs.id, curs.nume, curs.parinteId, curs.proprietarId};
}
}

ServerEdu::ServerEdu(std::uint16_t portNou, std::string adresaIpNoua)
    : ManagerSocket(std::move(adresaIpNoua), portNou) {
}

ServerEdu::ServerEdu(AutentificareService& autentificare,
                     CursService& cursuri,
                     std::uint16_t portNou,
                     std::string adresaIpNoua)
    : ManagerSocket(std::move(adresaIpNoua), portNou),
      autentificareService(&autentificare),
      cursService(&cursuri) {
}

ServerEdu::~ServerEdu() noexcept {
    opresteNod();
}

void ServerEdu::pornesteNod() {
    if (ruleaza) {
        throw ExceptieEdu("Serverul este deja pornit.");
    }
    SocketWindows socketNou(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!socketNou.esteValid()) {
        throw ExceptieEdu(eroareServer("Crearea socket-ului serverului a esuat"));
    }
    BOOL reutilizareAdresa = TRUE;
    if (setsockopt(socketNou.obtine(), SOL_SOCKET, SO_REUSEADDR,
                   reinterpret_cast<const char*>(&reutilizareAdresa),
                   sizeof(reutilizareAdresa)) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareServer("Configurarea socket-ului serverului a esuat"));
    }
    sockaddr_in adresa{};
    adresa.sin_family = AF_INET;
    adresa.sin_port = htons(port);
    if (InetPtonA(AF_INET, adresaIp.c_str(), &adresa.sin_addr) != 1) {
        throw ExceptieEdu("Adresa IPv4 a serverului este invalida.");
    }
    if (bind(socketNou.obtine(), reinterpret_cast<const sockaddr*>(&adresa),
             sizeof(adresa)) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareServer("Operatia bind a esuat"));
    }
    if (listen(socketNou.obtine(), SOMAXCONN) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareServer("Operatia listen a esuat"));
    }
    sockaddr_in adresaFinala{};
    int dimensiuneAdresa = sizeof(adresaFinala);
    if (getsockname(socketNou.obtine(), reinterpret_cast<sockaddr*>(&adresaFinala),
                    &dimensiuneAdresa) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareServer("Citirea portului serverului a esuat"));
    }
    port = ntohs(adresaFinala.sin_port);
    socketPrincipal = std::move(socketNou);
    ruleaza = true;
}

void ServerEdu::inchideClientActiv() noexcept {
    std::lock_guard<std::mutex> blocare(mutexSocketClient);
    if (socketClientActiv.esteValid()) {
        shutdown(socketClientActiv.obtine(), SD_BOTH);
        socketClientActiv.inchide();
    }
}

void ServerEdu::opresteNod() {
    ruleaza = false;
    if (socketPrincipal.esteValid()) {
        shutdown(socketPrincipal.obtine(), SD_BOTH);
        socketPrincipal.inchide();
    }
    inchideClientActiv();
}

void ServerEdu::proceseazaCerere() {
    if (!ruleaza || !socketPrincipal.esteValid()) {
        throw ExceptieEdu("Serverul nu este pornit.");
    }
    const SOCKET acceptat = accept(socketPrincipal.obtine(), nullptr, nullptr);
    if (acceptat == INVALID_SOCKET) {
        if (!ruleaza) {
            return;
        }
        throw ExceptieEdu(eroareServer("Acceptarea clientului a esuat"));
    }
    {
        std::lock_guard<std::mutex> blocare(mutexSocketClient);
        socketClientActiv.reseteaza(acceptat);
    }

    SesiuneClient sesiune;
    try {
        while (ruleaza) {
            SOCKET client{};
            {
                std::lock_guard<std::mutex> blocare(mutexSocketClient);
                if (!socketClientActiv.esteValid()) {
                    break;
                }
                client = socketClientActiv.obtine();
            }
            const auto cadru = primesteMesaj(client);
            if (!cadru.has_value()) {
                break;
            }

            CerereEdu cerere;
            try {
                cerere = ProtocolEdu::decodificaCerere(*cadru);
            } catch (const ExceptieEdu&) {
                const std::uint32_t idInvalid =
                    ProtocolEdu::extrageIdCerere(*cadru).value_or(1);
                trimiteMesaj(client, ProtocolEdu::codificaRaspuns(
                    raspuns(idInvalid, CodRezultatEdu::ProtocolInvalid,
                            "Mesaj de protocol invalid.")));
                continue;
            }

            bool solicitaDeconectare = false;
            const RaspunsEdu rezultat = distribuieCerere(
                cerere, sesiune, solicitaDeconectare);
            trimiteMesaj(client, ProtocolEdu::codificaRaspuns(rezultat));
            if (solicitaDeconectare) {
                break;
            }
        }
    } catch (...) {
        inchideClientActiv();
        if (!ruleaza) {
            return;
        }
        throw;
    }
    inchideClientActiv();
}

RaspunsEdu ServerEdu::distribuieCerere(const CerereEdu& cerere,
                                       SesiuneClient& sesiune,
                                       bool& solicitaDeconectare) {
    try {
        switch (cerere.tip) {
        case TipCerereEdu::Ping:
            verificaCampuri(cerere, {});
            return raspuns(cerere.idCerere, CodRezultatEdu::Succes, "PONG");
        case TipCerereEdu::Autentificare:
            return proceseazaAutentificare(cerere, sesiune);
        case TipCerereEdu::ListeazaCursuri:
            return proceseazaListeazaCursuri(cerere, sesiune);
        case TipCerereEdu::ObtineCurs:
            return proceseazaObtineCurs(cerere, sesiune);
        case TipCerereEdu::CreeazaCurs:
            return proceseazaCreeazaCurs(cerere, sesiune);
        case TipCerereEdu::ActualizeazaCurs:
            return proceseazaActualizeazaCurs(cerere, sesiune);
        case TipCerereEdu::StergeCurs:
            return proceseazaStergeCurs(cerere, sesiune);
        case TipCerereEdu::Deconectare:
            verificaCampuri(cerere, {});
            solicitaDeconectare = true;
            sesiune = {};
            return raspuns(cerere.idCerere, CodRezultatEdu::Succes, "DECONECTAT");
        default:
            return raspuns(cerere.idCerere, CodRezultatEdu::CerereNecunoscuta,
                           "Tip de cerere necunoscut.");
        }
    } catch (const ExceptieEdu& exceptie) {
        const CodRezultatEdu cod = clasificaExceptie(exceptie.what());
        return raspuns(cerere.idCerere, cod, mesajPublicPentru(cod));
    } catch (...) {
        return raspuns(cerere.idCerere, CodRezultatEdu::EroareInterna,
                       "Eroare interna a serverului.");
    }
}

RaspunsEdu ServerEdu::proceseazaAutentificare(const CerereEdu& cerere,
                                              SesiuneClient& sesiune) {
    verificaCampuri(cerere, {CampEdu::Email, CampEdu::Parola});
    if (sesiune.autentificat) {
        return raspuns(cerere.idCerere, CodRezultatEdu::AccesInterzis,
                       "Conexiunea este deja autentificata.");
    }
    if (autentificareService == nullptr) {
        return raspuns(cerere.idCerere, CodRezultatEdu::EroareInterna,
                       "Serviciul nu este disponibil.");
    }
    const auto rezultat = autentificareService->autentifica(
        campObligatoriu(cerere, CampEdu::Email),
        campObligatoriu(cerere, CampEdu::Parola));
    if (!rezultat.succes) {
        return raspuns(cerere.idCerere, CodRezultatEdu::AutentificareEsuata,
                       rezultat.mesajPublic);
    }
    sesiune.autentificat = true;
    sesiune.utilizatorId = *rezultat.utilizatorId;
    sesiune.rol = *rezultat.rol;
    RaspunsEdu raspunsAutentificare = raspuns(
        cerere.idCerere, CodRezultatEdu::Succes, rezultat.mesajPublic);
    raspunsAutentificare.campuri = {
        {static_cast<std::uint16_t>(CampEdu::UtilizatorId),
         std::to_string(sesiune.utilizatorId)},
        {static_cast<std::uint16_t>(CampEdu::Rol), sesiune.rol}};
    return raspunsAutentificare;
}

RaspunsEdu ServerEdu::proceseazaListeazaCursuri(
    const CerereEdu& cerere,
    const SesiuneClient& sesiune) {
    verificaCampuri(cerere, {});
    verificaAutentificare(sesiune);
    if (cursService == nullptr) {
        throw ExceptieEdu("Serviciul de cursuri nu este disponibil.");
    }
    RaspunsEdu rezultat = raspuns(
        cerere.idCerere, CodRezultatEdu::Succes, "Cursurile au fost citite.");
    for (const auto& curs : cursService->listeazaCursuri()) {
        rezultat.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Curs),
                                    ProtocolEdu::codificaCurs(cursPublic(curs))});
    }
    return rezultat;
}

RaspunsEdu ServerEdu::proceseazaObtineCurs(
    const CerereEdu& cerere,
    const SesiuneClient& sesiune) {
    verificaCampuri(cerere, {CampEdu::CursId});
    verificaAutentificare(sesiune);
    const int id = convertesteId(campObligatoriu(cerere, CampEdu::CursId),
                                 "Id-ul cursului");
    const auto curs = cursService->obtineCurs(id);
    if (!curs.has_value()) {
        return raspuns(cerere.idCerere, CodRezultatEdu::ResursaInexistenta,
                       "Cursul nu exista.");
    }
    RaspunsEdu rezultat = raspuns(
        cerere.idCerere, CodRezultatEdu::Succes, "Cursul a fost citit.");
    rezultat.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Curs),
                                ProtocolEdu::codificaCurs(cursPublic(*curs))});
    return rezultat;
}

RaspunsEdu ServerEdu::proceseazaCreeazaCurs(
    const CerereEdu& cerere,
    const SesiuneClient& sesiune) {
    verificaCampuri(cerere, {CampEdu::Nume, CampEdu::ParinteId,
                              CampEdu::ProprietarId});
    verificaAutentificare(sesiune);
    CerereCreareCurs creare;
    creare.actorId = sesiune.utilizatorId;
    creare.nume = campObligatoriu(cerere, CampEdu::Nume);
    if (const auto parinte = ProtocolEdu::cautaCamp(cerere.campuri, CampEdu::ParinteId)) {
        creare.parinteId = convertesteId(*parinte, "Id-ul cursului parinte");
    }
    if (sesiune.rol == "profesor") {
        creare.proprietarId = sesiune.utilizatorId;
    } else if (sesiune.rol == "administrator") {
        creare.proprietarId = convertesteId(
            campObligatoriu(cerere, CampEdu::ProprietarId), "Id-ul proprietarului");
    } else {
        throw ExceptieEdu("Studentul nu poate crea cursuri.");
    }
    const int id = cursService->creeazaCurs(creare);
    RaspunsEdu rezultat = raspuns(
        cerere.idCerere, CodRezultatEdu::Succes, "Cursul a fost creat.");
    rezultat.campuri.push_back({static_cast<std::uint16_t>(CampEdu::CursId),
                                std::to_string(id)});
    return rezultat;
}

RaspunsEdu ServerEdu::proceseazaActualizeazaCurs(
    const CerereEdu& cerere,
    const SesiuneClient& sesiune) {
    verificaCampuri(cerere, {CampEdu::CursId, CampEdu::Nume, CampEdu::ParinteId});
    verificaAutentificare(sesiune);
    CerereActualizareCurs actualizare;
    actualizare.actorId = sesiune.utilizatorId;
    actualizare.cursId = convertesteId(
        campObligatoriu(cerere, CampEdu::CursId), "Id-ul cursului");
    actualizare.nume = campObligatoriu(cerere, CampEdu::Nume);
    if (const auto parinte = ProtocolEdu::cautaCamp(cerere.campuri, CampEdu::ParinteId)) {
        actualizare.parinteId = convertesteId(*parinte, "Id-ul cursului parinte");
    }
    cursService->actualizeazaCurs(actualizare);
    return raspuns(cerere.idCerere, CodRezultatEdu::Succes,
                   "Cursul a fost actualizat.");
}

RaspunsEdu ServerEdu::proceseazaStergeCurs(
    const CerereEdu& cerere,
    const SesiuneClient& sesiune) {
    verificaCampuri(cerere, {CampEdu::CursId});
    verificaAutentificare(sesiune);
    const int id = convertesteId(campObligatoriu(cerere, CampEdu::CursId),
                                 "Id-ul cursului");
    cursService->stergeCurs(sesiune.utilizatorId, id);
    return raspuns(cerere.idCerere, CodRezultatEdu::Succes,
                   "Cursul a fost sters.");
}

bool ServerEdu::estePornit() const noexcept {
    return ruleaza;
}

std::uint16_t ServerEdu::obtinePort() const noexcept {
    return port;
}
