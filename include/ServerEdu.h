#pragma once

#include "ManagerSocket.h"
#include "ProtocolEdu.h"
#include "SesiuneClient.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

class AutentificareService;
class CursService;

class ServerEdu : public ManagerSocket {
private:
    std::atomic<bool> ruleaza{};
    AutentificareService* autentificareService{};
    CursService* cursService{};
    SocketWindows socketClientActiv;
    mutable std::mutex mutexSocketClient;

    RaspunsEdu distribuieCerere(const CerereEdu& cerere,
                                SesiuneClient& sesiune,
                                bool& solicitaDeconectare);
    RaspunsEdu proceseazaAutentificare(const CerereEdu& cerere,
                                       SesiuneClient& sesiune);
    RaspunsEdu proceseazaListeazaCursuri(const CerereEdu& cerere,
                                         const SesiuneClient& sesiune);
    RaspunsEdu proceseazaObtineCurs(const CerereEdu& cerere,
                                    const SesiuneClient& sesiune);
    RaspunsEdu proceseazaCreeazaCurs(const CerereEdu& cerere,
                                     const SesiuneClient& sesiune);
    RaspunsEdu proceseazaActualizeazaCurs(const CerereEdu& cerere,
                                          const SesiuneClient& sesiune);
    RaspunsEdu proceseazaStergeCurs(const CerereEdu& cerere,
                                    const SesiuneClient& sesiune);
    void inchideClientActiv() noexcept;

public:
    explicit ServerEdu(std::uint16_t port = 0,
                       std::string adresaIp = "127.0.0.1");
    ServerEdu(AutentificareService& autentificareService,
              CursService& cursService,
              std::uint16_t port = 0,
              std::string adresaIp = "127.0.0.1");
    ~ServerEdu() noexcept override;

    void pornesteNod() override;
    void opresteNod() override;
    void proceseazaCerere();

    bool estePornit() const noexcept;
    std::uint16_t obtinePort() const noexcept;
};
