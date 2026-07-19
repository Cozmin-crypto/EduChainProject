#pragma once

#include "ManagerSocket.h"
#include "ProtocolEdu.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class ClientEdu : public ManagerSocket {
private:
    bool conectat{};
    bool autentificat{};
    std::string rolAutentificat;
    std::string ultimulRaspuns;
    std::uint32_t urmatorulIdCerere{1};

    std::uint32_t genereazaIdCerere();
    static void verificaSucces(const RaspunsEdu& raspuns);

public:
    explicit ClientEdu(std::uint16_t port = 0,
                       std::string adresaIp = "127.0.0.1");
    ~ClientEdu() noexcept override;

    void pornesteNod() override;
    void opresteNod() override;
    void trimiteCerere();
    std::string trimiteCerere(const std::string& mesaj);
    RaspunsEdu executaCerere(CerereEdu cerere);

    RaspunsEdu autentifica(const std::string& email, const std::string& parola);
    std::vector<CursPublicEdu> listeazaCursuri();
    std::optional<CursPublicEdu> obtineCurs(int cursId);
    int creeazaCurs(const std::string& nume,
                    std::optional<int> parinteId = std::nullopt,
                    std::optional<int> proprietarId = std::nullopt);
    void actualizeazaCurs(int cursId,
                          const std::string& nume,
                          std::optional<int> parinteId = std::nullopt);
    void stergeCurs(int cursId);
    void deconecteaza();

    bool esteConectat() const noexcept;
    bool esteAutentificat() const noexcept;
    const std::string& obtineRolAutentificat() const noexcept;
    const std::string& obtineUltimulRaspuns() const noexcept;
};
