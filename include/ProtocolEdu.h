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
    Deconectare = 8,
    ListeazaLectii = 9,
    ObtineLectie = 10,
    CreeazaLectieText = 11,
    CreeazaLectieVideo = 12,
    ActualizeazaLectie = 13,
    StergeLectie = 14,
    ListeazaEvaluari = 15,
    ObtineEvaluare = 16,
    CreeazaChestionar = 17,
    CreeazaExamenFinal = 18,
    ActualizeazaEvaluare = 19,
    StergeEvaluare = 20,
    ListeazaIntrebari = 21,
    AdaugaIntrebare = 22,
    ActualizeazaIntrebare = 23,
    StergeIntrebare = 24
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
    Curs = 9,
    LectieId = 10,
    TipLectie = 11,
    Continut = 12,
    DimensiuneOcteti = 13,
    NumarCuvinte = 14,
    Durata = 15,
    Codec = 16,
    Lectie = 17,
    EvaluareId = 18,
    TipEvaluare = 19,
    LimitaTimp = 20,
    EsteObligatorie = 21,
    NumarIntrebari = 22,
    Pondere = 23,
    Evaluare = 24,
    IntrebareId = 25,
    Enunt = 26,
    PunctajMaxim = 27,
    Ordine = 28,
    Intrebare = 29
};

enum class TipLectieEdu : std::uint16_t {
    Text = 1,
    Video = 2
};

enum class TipEvaluareEdu : std::uint16_t {
    Chestionar = 1,
    ExamenFinal = 2
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

struct LectiePublicEdu {
    int id{};
    int cursId{};
    std::string nume;
    TipLectieEdu tip{TipLectieEdu::Text};
    std::string continut;
    long long dimensiuneOcteti{};
    std::optional<long long> numarCuvinte;
    std::optional<long long> durata;
    std::optional<std::string> codec;
};

struct EvaluarePublicEdu {
    int id{};
    int cursId{};
    int profesorId{};
    std::string nume;
    TipEvaluareEdu tip{TipEvaluareEdu::Chestionar};
    long long limitaTimp{};
    bool esteObligatorie{};
    std::optional<long long> numarIntrebari;
    std::optional<double> pondere;
};

struct IntrebarePublicEdu {
    int id{};
    int evaluareId{};
    std::string enunt;
    double punctajMaxim{};
    long long ordine{};
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
    static std::string codificaLectie(const LectiePublicEdu& lectie);
    static LectiePublicEdu decodificaLectie(const std::string& date);
    static std::string codificaEvaluare(const EvaluarePublicEdu& evaluare);
    static EvaluarePublicEdu decodificaEvaluare(const std::string& date);
    static std::string codificaIntrebare(const IntrebarePublicEdu& intrebare);
    static IntrebarePublicEdu decodificaIntrebare(const std::string& date);
};
