#include "ManagerSocket.h"

#include "ExceptieEdu.h"

#include <algorithm>
#include <atomic>
#include <limits>
#include <utility>

#pragma comment(lib, "Ws2_32.lib")

namespace {
std::atomic<int> instanteWinsockActive{};
std::atomic<int> socketuriActive{};
constexpr std::uint32_t dimensiuneMaximaMesaj = 16U * 1024U * 1024U;

std::string mesajEroareWinsock(const std::string& context) {
    return context + " (cod Winsock " + std::to_string(WSAGetLastError()) + ").";
}
}

InitializatorWinsock::InitializatorWinsock() {
    WSADATA dateWinsock{};
    const int rezultat = WSAStartup(MAKEWORD(2, 2), &dateWinsock);
    if (rezultat != 0) {
        throw ExceptieEdu("Initializarea Winsock a esuat (cod " +
                          std::to_string(rezultat) + ").");
    }
    if (LOBYTE(dateWinsock.wVersion) != 2 || HIBYTE(dateWinsock.wVersion) != 2) {
        WSACleanup();
        throw ExceptieEdu("Versiunea Winsock 2.2 nu este disponibila.");
    }
    ++instanteWinsockActive;
}

InitializatorWinsock::~InitializatorWinsock() noexcept {
    WSACleanup();
    --instanteWinsockActive;
}

int InitializatorWinsock::numarInstanteActive() noexcept {
    return instanteWinsockActive.load();
}

SocketWindows::SocketWindows(SOCKET socketNou) noexcept
    : socket(socketNou) {
    if (esteValid()) {
        ++socketuriActive;
    }
}

SocketWindows::~SocketWindows() noexcept {
    inchide();
}

SocketWindows::SocketWindows(SocketWindows&& altul) noexcept
    : socket(altul.socket) {
    altul.socket = INVALID_SOCKET;
}

SocketWindows& SocketWindows::operator=(SocketWindows&& altul) noexcept {
    if (this != &altul) {
        inchide();
        socket = altul.socket;
        altul.socket = INVALID_SOCKET;
    }
    return *this;
}

SOCKET SocketWindows::obtine() const noexcept {
    return socket;
}

bool SocketWindows::esteValid() const noexcept {
    return socket != INVALID_SOCKET;
}

void SocketWindows::inchide() noexcept {
    if (esteValid()) {
        closesocket(socket);
        socket = INVALID_SOCKET;
        --socketuriActive;
    }
}

void SocketWindows::reseteaza(SOCKET socketNou) noexcept {
    if (socket == socketNou) {
        return;
    }
    inchide();
    socket = socketNou;
    if (esteValid()) {
        ++socketuriActive;
    }
}

int SocketWindows::numarSocketuriActive() noexcept {
    return socketuriActive.load();
}

ManagerSocket::ManagerSocket(std::string adresaIpNoua, std::uint16_t portNou)
    : adresaIp(std::move(adresaIpNoua)), port(portNou) {
}

void ManagerSocket::sendAll(SOCKET socket,
                            const void* date,
                            std::size_t dimensiune) {
    if (socket == INVALID_SOCKET || (date == nullptr && dimensiune != 0)) {
        throw ExceptieEdu("Parametrii pentru trimiterea datelor sunt invalizi.");
    }
    const auto* cursor = static_cast<const char*>(date);
    std::size_t trimise = 0;
    while (trimise < dimensiune) {
        const std::size_t ramase = dimensiune - trimise;
        const int segment = static_cast<int>(std::min<std::size_t>(
            ramase, static_cast<std::size_t>(std::numeric_limits<int>::max())));
        const int rezultat = send(socket, cursor + trimise, segment, 0);
        if (rezultat == SOCKET_ERROR) {
            throw ExceptieEdu(mesajEroareWinsock("Trimiterea datelor a esuat"));
        }
        if (rezultat == 0) {
            throw ExceptieEdu("Conexiunea s-a inchis in timpul trimiterii datelor.");
        }
        trimise += static_cast<std::size_t>(rezultat);
    }
}

bool ManagerSocket::recvAll(SOCKET socket,
                            void* destinatie,
                            std::size_t dimensiune) {
    if (socket == INVALID_SOCKET || (destinatie == nullptr && dimensiune != 0)) {
        throw ExceptieEdu("Parametrii pentru primirea datelor sunt invalizi.");
    }
    auto* cursor = static_cast<char*>(destinatie);
    std::size_t primite = 0;
    while (primite < dimensiune) {
        const std::size_t ramase = dimensiune - primite;
        const int segment = static_cast<int>(std::min<std::size_t>(
            ramase, static_cast<std::size_t>(std::numeric_limits<int>::max())));
        const int rezultat = recv(socket, cursor + primite, segment, 0);
        if (rezultat == SOCKET_ERROR) {
            throw ExceptieEdu(mesajEroareWinsock("Primirea datelor a esuat"));
        }
        if (rezultat == 0) {
            if (primite == 0) {
                return false;
            }
            throw ExceptieEdu("Conexiunea s-a inchis in mijlocul unui mesaj.");
        }
        primite += static_cast<std::size_t>(rezultat);
    }
    return true;
}

void ManagerSocket::trimiteMesaj(SOCKET socket, const std::string& mesaj) {
    if (mesaj.size() > dimensiuneMaximaMesaj) {
        throw ExceptieEdu("Mesajul depaseste dimensiunea maxima permisa.");
    }
    const std::uint32_t lungimeRetea = htonl(static_cast<std::uint32_t>(mesaj.size()));
    sendAll(socket, &lungimeRetea, sizeof(lungimeRetea));
    if (!mesaj.empty()) {
        sendAll(socket, mesaj.data(), mesaj.size());
    }
}

std::optional<std::string> ManagerSocket::primesteMesaj(SOCKET socket) {
    std::uint32_t lungimeRetea{};
    if (!recvAll(socket, &lungimeRetea, sizeof(lungimeRetea))) {
        return std::nullopt;
    }
    const std::uint32_t lungime = ntohl(lungimeRetea);
    if (lungime > dimensiuneMaximaMesaj) {
        throw ExceptieEdu("Mesajul primit depaseste dimensiunea maxima permisa.");
    }
    std::string mesaj(lungime, '\0');
    if (lungime != 0 && !recvAll(socket, mesaj.data(), mesaj.size())) {
        throw ExceptieEdu("Conexiunea s-a inchis inainte de corpul mesajului.");
    }
    return mesaj;
}
