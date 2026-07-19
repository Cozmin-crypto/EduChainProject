#include "ClientEdu.h"

#include "ExceptieEdu.h"

#include <WS2tcpip.h>

#include <limits>
#include <utility>

namespace {
std::string eroareClient(const std::string& context) {
    return context + " (cod Winsock " + std::to_string(WSAGetLastError()) + ").";
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
}

ClientEdu::ClientEdu(std::uint16_t portNou, std::string adresaIpNoua)
    : ManagerSocket(std::move(adresaIpNoua), portNou) {
}

ClientEdu::~ClientEdu() noexcept {
    opresteNod();
}

void ClientEdu::pornesteNod() {
    if (conectat) {
        throw ExceptieEdu("Clientul este deja conectat.");
    }
    if (port == 0) {
        throw ExceptieEdu("Portul serverului trebuie sa fie strict pozitiv.");
    }
    SocketWindows socketNou(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!socketNou.esteValid()) {
        throw ExceptieEdu(eroareClient("Crearea socket-ului clientului a esuat"));
    }
    sockaddr_in adresaServer{};
    adresaServer.sin_family = AF_INET;
    adresaServer.sin_port = htons(port);
    if (InetPtonA(AF_INET, adresaIp.c_str(), &adresaServer.sin_addr) != 1) {
        throw ExceptieEdu("Adresa IPv4 a serverului este invalida.");
    }
    if (connect(socketNou.obtine(), reinterpret_cast<const sockaddr*>(&adresaServer),
                sizeof(adresaServer)) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareClient("Conectarea la server a esuat"));
    }
    socketPrincipal = std::move(socketNou);
    conectat = true;
    autentificat = false;
    rolAutentificat.clear();
}

void ClientEdu::opresteNod() {
    conectat = false;
    autentificat = false;
    rolAutentificat.clear();
    if (socketPrincipal.esteValid()) {
        shutdown(socketPrincipal.obtine(), SD_BOTH);
        socketPrincipal.inchide();
    }
}

std::uint32_t ClientEdu::genereazaIdCerere() {
    if (urmatorulIdCerere == 0) {
        urmatorulIdCerere = 1;
    }
    return urmatorulIdCerere++;
}

RaspunsEdu ClientEdu::executaCerere(CerereEdu cerere) {
    if (!conectat || !socketPrincipal.esteValid()) {
        throw ExceptieEdu("Clientul nu este conectat la server.");
    }
    if (cerere.idCerere == 0) {
        cerere.idCerere = genereazaIdCerere();
    }
    try {
        trimiteMesaj(socketPrincipal.obtine(), ProtocolEdu::codificaCerere(cerere));
        const auto cadru = primesteMesaj(socketPrincipal.obtine());
        if (!cadru.has_value()) {
            opresteNod();
            throw ExceptieEdu("Serverul a inchis conexiunea fara raspuns.");
        }
        RaspunsEdu raspuns = ProtocolEdu::decodificaRaspuns(*cadru);
        if (raspuns.idCerere != cerere.idCerere) {
            throw ExceptieEdu("ID-ul raspunsului nu corespunde cererii.");
        }
        ultimulRaspuns = raspuns.mesajPublic;
        return raspuns;
    } catch (...) {
        if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAESHUTDOWN ||
            WSAGetLastError() == WSAENOTCONN) {
            opresteNod();
        }
        throw;
    }
}

void ClientEdu::trimiteCerere() {
    ultimulRaspuns = trimiteCerere("PING");
}

std::string ClientEdu::trimiteCerere(const std::string& mesaj) {
    CerereEdu cerere;
    cerere.tip = mesaj == "PING"
        ? TipCerereEdu::Ping
        : static_cast<TipCerereEdu>(std::numeric_limits<std::uint16_t>::max());
    const auto raspuns = executaCerere(std::move(cerere));
    return raspuns.mesajPublic;
}

RaspunsEdu ClientEdu::autentifica(const std::string& email,
                                  const std::string& parola) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::Autentificare;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::Email), email},
        {static_cast<std::uint16_t>(CampEdu::Parola), parola}};
    RaspunsEdu raspuns = executaCerere(std::move(cerere));
    if (raspuns.cod == CodRezultatEdu::Succes) {
        const auto rol = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Rol);
        const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::UtilizatorId);
        if (!rol || !id) {
            throw ExceptieEdu("Raspunsul autentificarii este incomplet.");
        }
        convertesteId(*id, "Id-ul utilizatorului autentificat");
        autentificat = true;
        rolAutentificat = *rol;
    }
    return raspuns;
}

void ClientEdu::verificaSucces(const RaspunsEdu& raspuns) {
    if (raspuns.cod != CodRezultatEdu::Succes) {
        throw ExceptieEdu(raspuns.mesajPublic);
    }
}

std::vector<CursPublicEdu> ClientEdu::listeazaCursuri() {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ListeazaCursuri;
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    std::vector<CursPublicEdu> cursuri;
    for (const auto& camp : raspuns.campuri) {
        if (camp.id != static_cast<std::uint16_t>(CampEdu::Curs)) {
            throw ExceptieEdu("Raspunsul listei de cursuri contine un camp invalid.");
        }
        cursuri.push_back(ProtocolEdu::decodificaCurs(camp.valoare));
    }
    return cursuri;
}

std::optional<CursPublicEdu> ClientEdu::obtineCurs(int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ObtineCurs;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                       std::to_string(cursId)}};
    const auto raspuns = executaCerere(std::move(cerere));
    if (raspuns.cod == CodRezultatEdu::ResursaInexistenta) {
        return std::nullopt;
    }
    verificaSucces(raspuns);
    const auto curs = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Curs);
    if (!curs) {
        throw ExceptieEdu("Raspunsul nu contine cursul solicitat.");
    }
    return ProtocolEdu::decodificaCurs(*curs);
}

int ClientEdu::creeazaCurs(const std::string& nume,
                           std::optional<int> parinteId,
                           std::optional<int> proprietarId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::CreeazaCurs;
    cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Nume), nume});
    if (parinteId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ParinteId),
                                  std::to_string(*parinteId)});
    }
    if (proprietarId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ProprietarId),
                                  std::to_string(*proprietarId)});
    }
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::CursId);
    if (!id) {
        throw ExceptieEdu("Raspunsul nu contine ID-ul cursului creat.");
    }
    return convertesteId(*id, "Id-ul cursului creat");
}

void ClientEdu::actualizeazaCurs(int cursId,
                                 const std::string& nume,
                                 std::optional<int> parinteId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ActualizeazaCurs;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), nume}};
    if (parinteId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ParinteId),
                                  std::to_string(*parinteId)});
    }
    verificaSucces(executaCerere(std::move(cerere)));
}

void ClientEdu::stergeCurs(int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::StergeCurs;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                       std::to_string(cursId)}};
    verificaSucces(executaCerere(std::move(cerere)));
}

void ClientEdu::deconecteaza() {
    if (!conectat) {
        return;
    }
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::Deconectare;
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    opresteNod();
}

bool ClientEdu::esteConectat() const noexcept {
    return conectat;
}

bool ClientEdu::esteAutentificat() const noexcept {
    return autentificat;
}

const std::string& ClientEdu::obtineRolAutentificat() const noexcept {
    return rolAutentificat;
}

const std::string& ClientEdu::obtineUltimulRaspuns() const noexcept {
    return ultimulRaspuns;
}
