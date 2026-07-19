#include "ProtocolEdu.h"

#include "ExceptieEdu.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>

#include <cstring>
#include <limits>

namespace {
void adaugaU16(std::string& rezultat, std::uint16_t valoare) {
    const std::uint16_t retea = htons(valoare);
    rezultat.append(reinterpret_cast<const char*>(&retea), sizeof(retea));
}

void adaugaU32(std::string& rezultat, std::uint32_t valoare) {
    const std::uint32_t retea = htonl(valoare);
    rezultat.append(reinterpret_cast<const char*>(&retea), sizeof(retea));
}

std::uint16_t citesteU16(const std::string& date, std::size_t& pozitie) {
    if (date.size() - pozitie < sizeof(std::uint16_t)) {
        throw ExceptieEdu("Mesajul protocolului este trunchiat.");
    }
    std::uint16_t retea{};
    std::memcpy(&retea, date.data() + pozitie, sizeof(retea));
    pozitie += sizeof(retea);
    return ntohs(retea);
}

std::uint32_t citesteU32(const std::string& date, std::size_t& pozitie) {
    if (date.size() - pozitie < sizeof(std::uint32_t)) {
        throw ExceptieEdu("Mesajul protocolului este trunchiat.");
    }
    std::uint32_t retea{};
    std::memcpy(&retea, date.data() + pozitie, sizeof(retea));
    pozitie += sizeof(retea);
    return ntohl(retea);
}

void adaugaText(std::string& rezultat, const std::string& text) {
    if (text.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw ExceptieEdu("Campul protocolului este prea mare.");
    }
    adaugaU32(rezultat, static_cast<std::uint32_t>(text.size()));
    rezultat.append(text);
}

std::string citesteText(const std::string& date, std::size_t& pozitie) {
    const std::uint32_t lungime = citesteU32(date, pozitie);
    if (lungime > date.size() - pozitie) {
        throw ExceptieEdu("Lungimea campului protocolului este invalida.");
    }
    std::string rezultat(date.data() + pozitie, lungime);
    pozitie += lungime;
    return rezultat;
}

void adaugaCampuri(std::string& rezultat,
                   const std::vector<CampProtocolEdu>& campuri) {
    if (campuri.size() > std::numeric_limits<std::uint16_t>::max()) {
        throw ExceptieEdu("Mesajul contine prea multe campuri.");
    }
    adaugaU16(rezultat, static_cast<std::uint16_t>(campuri.size()));
    for (const auto& camp : campuri) {
        adaugaU16(rezultat, camp.id);
        adaugaText(rezultat, camp.valoare);
    }
}

std::vector<CampProtocolEdu> citesteCampuri(const std::string& date,
                                            std::size_t& pozitie) {
    const std::uint16_t numarCampuri = citesteU16(date, pozitie);
    std::vector<CampProtocolEdu> campuri;
    campuri.reserve(numarCampuri);
    for (std::uint16_t index = 0; index < numarCampuri; ++index) {
        const std::uint16_t id = citesteU16(date, pozitie);
        campuri.push_back({id, citesteText(date, pozitie)});
    }
    return campuri;
}

int convertesteIntregPozitiv(const std::string& valoare,
                             const std::string& denumire) {
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

std::string ProtocolEdu::codificaCerere(const CerereEdu& cerere) {
    std::string rezultat;
    adaugaU16(rezultat, cerere.versiune);
    adaugaU16(rezultat, static_cast<std::uint16_t>(cerere.tip));
    adaugaU32(rezultat, cerere.idCerere);
    adaugaCampuri(rezultat, cerere.campuri);
    return rezultat;
}

CerereEdu ProtocolEdu::decodificaCerere(const std::string& date) {
    std::size_t pozitie = 0;
    CerereEdu cerere;
    cerere.versiune = citesteU16(date, pozitie);
    cerere.tip = static_cast<TipCerereEdu>(citesteU16(date, pozitie));
    cerere.idCerere = citesteU32(date, pozitie);
    cerere.campuri = citesteCampuri(date, pozitie);
    if (pozitie != date.size()) {
        throw ExceptieEdu("Mesajul contine date suplimentare neasteptate.");
    }
    if (cerere.versiune != versiuneProtocolEdu || cerere.idCerere == 0) {
        throw ExceptieEdu("Versiunea sau ID-ul cererii este invalid.");
    }
    return cerere;
}

std::string ProtocolEdu::codificaRaspuns(const RaspunsEdu& raspuns) {
    std::string rezultat;
    adaugaU16(rezultat, raspuns.versiune);
    adaugaU32(rezultat, raspuns.idCerere);
    adaugaU16(rezultat, static_cast<std::uint16_t>(raspuns.cod));
    adaugaText(rezultat, raspuns.mesajPublic);
    adaugaCampuri(rezultat, raspuns.campuri);
    return rezultat;
}

RaspunsEdu ProtocolEdu::decodificaRaspuns(const std::string& date) {
    std::size_t pozitie = 0;
    RaspunsEdu raspuns;
    raspuns.versiune = citesteU16(date, pozitie);
    raspuns.idCerere = citesteU32(date, pozitie);
    raspuns.cod = static_cast<CodRezultatEdu>(citesteU16(date, pozitie));
    raspuns.mesajPublic = citesteText(date, pozitie);
    raspuns.campuri = citesteCampuri(date, pozitie);
    if (pozitie != date.size() || raspuns.versiune != versiuneProtocolEdu ||
        raspuns.idCerere == 0) {
        throw ExceptieEdu("Raspunsul protocolului este invalid.");
    }
    return raspuns;
}

std::optional<std::uint32_t> ProtocolEdu::extrageIdCerere(
    const std::string& date) noexcept {
    constexpr std::size_t pozitieId = sizeof(std::uint16_t) * 2;
    if (date.size() < pozitieId + sizeof(std::uint32_t)) {
        return std::nullopt;
    }
    std::uint32_t retea{};
    std::memcpy(&retea, date.data() + pozitieId, sizeof(retea));
    const std::uint32_t id = ntohl(retea);
    return id == 0 ? std::nullopt : std::optional<std::uint32_t>(id);
}

std::optional<std::string> ProtocolEdu::cautaCamp(
    const std::vector<CampProtocolEdu>& campuri,
    CampEdu id) {
    for (const auto& camp : campuri) {
        if (camp.id == static_cast<std::uint16_t>(id)) {
            return camp.valoare;
        }
    }
    return std::nullopt;
}

std::string ProtocolEdu::codificaCurs(const CursPublicEdu& curs) {
    std::string rezultat;
    std::vector<CampProtocolEdu> campuri{
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(curs.id)},
        {static_cast<std::uint16_t>(CampEdu::Nume), curs.nume},
        {static_cast<std::uint16_t>(CampEdu::ProprietarId),
         std::to_string(curs.proprietarId)}};
    if (curs.parinteId.has_value()) {
        campuri.push_back({static_cast<std::uint16_t>(CampEdu::ParinteId),
                           std::to_string(*curs.parinteId)});
    }
    adaugaCampuri(rezultat, campuri);
    return rezultat;
}

CursPublicEdu ProtocolEdu::decodificaCurs(const std::string& date) {
    std::size_t pozitie = 0;
    const auto campuri = citesteCampuri(date, pozitie);
    if (pozitie != date.size()) {
        throw ExceptieEdu("Datele cursului sunt invalide.");
    }
    const auto id = cautaCamp(campuri, CampEdu::CursId);
    const auto nume = cautaCamp(campuri, CampEdu::Nume);
    const auto proprietar = cautaCamp(campuri, CampEdu::ProprietarId);
    if (!id || !nume || !proprietar) {
        throw ExceptieEdu("Datele cursului sunt incomplete.");
    }
    CursPublicEdu curs;
    curs.id = convertesteIntregPozitiv(*id, "Id-ul cursului");
    curs.nume = *nume;
    curs.proprietarId = convertesteIntregPozitiv(*proprietar, "Id-ul proprietarului");
    if (const auto parinte = cautaCamp(campuri, CampEdu::ParinteId)) {
        curs.parinteId = convertesteIntregPozitiv(*parinte, "Id-ul parintelui");
    }
    return curs;
}
