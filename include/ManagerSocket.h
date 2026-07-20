#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "INodRetea.h"

class InitializatorWinsock {
public:
    InitializatorWinsock();
    ~InitializatorWinsock() noexcept;

    InitializatorWinsock(const InitializatorWinsock&) = delete;
    InitializatorWinsock& operator=(const InitializatorWinsock&) = delete;

    static int numarInstanteActive() noexcept;
};

class SocketWindows {
private:
    SOCKET socket{INVALID_SOCKET};

public:
    SocketWindows() noexcept = default;
    explicit SocketWindows(SOCKET socketNou) noexcept;
    ~SocketWindows() noexcept;

    SocketWindows(const SocketWindows&) = delete;
    SocketWindows& operator=(const SocketWindows&) = delete;
    SocketWindows(SocketWindows&& altul) noexcept;
    SocketWindows& operator=(SocketWindows&& altul) noexcept;

    SOCKET obtine() const noexcept;
    bool esteValid() const noexcept;
    void inchide() noexcept;
    void reseteaza(SOCKET socketNou = INVALID_SOCKET) noexcept;

    static int numarSocketuriActive() noexcept;
};

class ManagerSocket : public INodRetea {
protected:
    InitializatorWinsock initializatorWinsock;
    SocketWindows socketPrincipal;
    std::string adresaIp;
    std::uint16_t port{};

    ManagerSocket(std::string adresaIp, std::uint16_t port);

public:
    static void sendAll(SOCKET socket,
                        const void* date,
                        std::size_t dimensiune);
    static bool recvAll(SOCKET socket,
                        void* destinatie,
                        std::size_t dimensiune);
    static void trimiteMesaj(SOCKET socket, const std::string& mesaj);
    static std::optional<std::string> primesteMesaj(SOCKET socket);
};
