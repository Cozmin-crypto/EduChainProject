#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

constexpr std::uint16_t versiuneProtocolEdu = 1;

enum class TipCerereEdu : std::uint16_t {
    Ping = 1,
    Autentificare = 2,
    ListeazaCursuri = 3,
    ObtineCurs = 4,
    CreeazaCurs = 5,
    ActualizeazaCurs = 6,
    StergeCurs = 7,
    Deconectare = 8
};

enum class CodRezultatEdu : std::uint16_t {
    Succes = 0,
    AutentificareEsuata = 1,
    AccesInterzis = 2,
    ResursaInexistenta = 3,
    ValidareEsuata = 4,
    Conflict = 5,
    CerereNecunoscuta = 6,
    ProtocolInvalid = 7,
    EroareInterna = 8
};

enum class CampEdu : std::uint16_t {
    Email = 1,
    Parola = 2,
    CursId = 3,
    Nume = 4,
    ParinteId = 5,
    ProprietarId = 6,
    UtilizatorId = 7,
    Rol = 8,
    Curs = 9
};

struct CampProtocolEdu {
    std::uint16_t id{};
    std::string valoare;
};

struct CerereEdu {
    std::uint16_t versiune{versiuneProtocolEdu};
    TipCerereEdu tip{TipCerereEdu::Ping};
    std::uint32_t idCerere{};
    std::vector<CampProtocolEdu> campuri;
};

struct RaspunsEdu {
    std::uint16_t versiune{versiuneProtocolEdu};
    std::uint32_t idCerere{};
    CodRezultatEdu cod{CodRezultatEdu::Succes};
    std::string mesajPublic;
    std::vector<CampProtocolEdu> campuri;
};

struct CursPublicEdu {
    int id{};
    std::string nume;
    std::optional<int> parinteId;
    int proprietarId{};
};

class ProtocolEdu {
public:
    static std::string codificaCerere(const CerereEdu& cerere);
    static CerereEdu decodificaCerere(const std::string& date);
    static std::string codificaRaspuns(const RaspunsEdu& raspuns);
    static RaspunsEdu decodificaRaspuns(const std::string& date);
    static std::optional<std::uint32_t> extrageIdCerere(const std::string& date) noexcept;

    static std::optional<std::string> cautaCamp(
        const std::vector<CampProtocolEdu>& campuri,
        CampEdu id);
    static std::string codificaCurs(const CursPublicEdu& curs);
    static CursPublicEdu decodificaCurs(const std::string& date);
};
