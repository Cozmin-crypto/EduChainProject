#include "ProtocolEdu.h"

#include "ExceptieEdu.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <WinSock2.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <cmath>

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
    constexpr std::size_t dimensiuneMinimaCamp =
        sizeof(std::uint16_t) + sizeof(std::uint32_t);
    if (static_cast<std::size_t>(numarCampuri) >
        (date.size() - pozitie) / dimensiuneMinimaCamp) {
        throw ExceptieEdu("Numarul campurilor depaseste datele disponibile.");
    }
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

long long convertesteIntregNenegativ(const std::string& valoare,
                                     const std::string& denumire) {
    try {
        std::size_t convertite = 0;
        const long long rezultat = std::stoll(valoare, &convertite);
        if (convertite != valoare.size() || rezultat < 0) {
            throw ExceptieEdu(denumire + " este invalid.");
        }
        return rezultat;
    } catch (const ExceptieEdu&) {
        throw;
    } catch (...) {
        throw ExceptieEdu(denumire + " este invalid.");
    }
}

TipLectieEdu convertesteTipLectie(const std::string& valoare) {
    const long long tip = convertesteIntregNenegativ(valoare, "Tipul lectiei");
    if (tip == static_cast<long long>(TipLectieEdu::Text)) {
        return TipLectieEdu::Text;
    }
    if (tip == static_cast<long long>(TipLectieEdu::Video)) {
        return TipLectieEdu::Video;
    }
    throw ExceptieEdu("Tipul lectiei este invalid.");
}

double convertesteRealNenegativ(const std::string& valoare,
                                const std::string& denumire) {
    try {
        std::size_t convertite = 0;
        const double rezultat = std::stod(valoare, &convertite);
        if (convertite != valoare.size() || !std::isfinite(rezultat) || rezultat < 0.0) {
            throw ExceptieEdu(denumire + " este invalid.");
        }
        return rezultat;
    } catch (const ExceptieEdu&) { throw; }
    catch (...) { throw ExceptieEdu(denumire + " este invalid."); }
}

void verificaCampuriExacte(const std::vector<CampProtocolEdu>& campuri,
                           std::initializer_list<CampEdu> permise,
                           const std::string& entitate) {
    for (const auto& camp : campuri) {
        if (std::none_of(permise.begin(), permise.end(), [&](CampEdu permis) {
                return camp.id == static_cast<std::uint16_t>(permis);
            })) {
            throw ExceptieEdu("Datele " + entitate + " contin un camp necunoscut.");
        }
        if (std::count_if(campuri.begin(), campuri.end(), [&](const auto& altul) {
                return altul.id == camp.id;
            }) != 1) {
            throw ExceptieEdu("Datele " + entitate + " contin campuri duplicate.");
        }
    }
}

void verificaCampuriLectie(const std::vector<CampProtocolEdu>& campuri) {
    const std::uint16_t permise[] = {
        static_cast<std::uint16_t>(CampEdu::LectieId),
        static_cast<std::uint16_t>(CampEdu::CursId),
        static_cast<std::uint16_t>(CampEdu::Nume),
        static_cast<std::uint16_t>(CampEdu::TipLectie),
        static_cast<std::uint16_t>(CampEdu::Continut),
        static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti),
        static_cast<std::uint16_t>(CampEdu::NumarCuvinte),
        static_cast<std::uint16_t>(CampEdu::Durata),
        static_cast<std::uint16_t>(CampEdu::Codec)};
    for (const auto& camp : campuri) {
        if (std::find(std::begin(permise), std::end(permise), camp.id) ==
            std::end(permise)) {
            throw ExceptieEdu("Datele lectiei contin un camp necunoscut.");
        }
        if (std::count_if(campuri.begin(), campuri.end(), [&](const auto& altul) {
                return altul.id == camp.id;
            }) != 1) {
            throw ExceptieEdu("Datele lectiei contin campuri duplicate.");
        }
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

std::string ProtocolEdu::codificaLectie(const LectiePublicEdu& lectie) {
    std::vector<CampProtocolEdu> campuri{
        {static_cast<std::uint16_t>(CampEdu::LectieId), std::to_string(lectie.id)},
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(lectie.cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), lectie.nume},
        {static_cast<std::uint16_t>(CampEdu::TipLectie),
         std::to_string(static_cast<std::uint16_t>(lectie.tip))},
        {static_cast<std::uint16_t>(CampEdu::Continut), lectie.continut},
        {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti),
         std::to_string(lectie.dimensiuneOcteti)}};
    if (lectie.numarCuvinte) {
        campuri.push_back({static_cast<std::uint16_t>(CampEdu::NumarCuvinte),
                           std::to_string(*lectie.numarCuvinte)});
    }
    if (lectie.durata) {
        campuri.push_back({static_cast<std::uint16_t>(CampEdu::Durata),
                           std::to_string(*lectie.durata)});
    }
    if (lectie.codec) {
        campuri.push_back({static_cast<std::uint16_t>(CampEdu::Codec), *lectie.codec});
    }
    std::string rezultat;
    adaugaCampuri(rezultat, campuri);
    return rezultat;
}

LectiePublicEdu ProtocolEdu::decodificaLectie(const std::string& date) {
    std::size_t pozitie = 0;
    const auto campuri = citesteCampuri(date, pozitie);
    if (pozitie != date.size()) {
        throw ExceptieEdu("Datele lectiei contin octeti suplimentari.");
    }
    verificaCampuriLectie(campuri);
    const auto id = cautaCamp(campuri, CampEdu::LectieId);
    const auto cursId = cautaCamp(campuri, CampEdu::CursId);
    const auto nume = cautaCamp(campuri, CampEdu::Nume);
    const auto tip = cautaCamp(campuri, CampEdu::TipLectie);
    const auto continut = cautaCamp(campuri, CampEdu::Continut);
    const auto dimensiune = cautaCamp(campuri, CampEdu::DimensiuneOcteti);
    if (!id || !cursId || !nume || !tip || !continut || !dimensiune) {
        throw ExceptieEdu("Datele lectiei sunt incomplete.");
    }

    LectiePublicEdu lectie;
    lectie.id = convertesteIntregPozitiv(*id, "Id-ul lectiei");
    lectie.cursId = convertesteIntregPozitiv(*cursId, "Id-ul cursului");
    lectie.nume = *nume;
    lectie.tip = convertesteTipLectie(*tip);
    lectie.continut = *continut;
    lectie.dimensiuneOcteti = convertesteIntregNenegativ(
        *dimensiune, "Dimensiunea lectiei");
    if (const auto cuvinte = cautaCamp(campuri, CampEdu::NumarCuvinte)) {
        lectie.numarCuvinte = convertesteIntregNenegativ(*cuvinte, "Numarul de cuvinte");
    }
    if (const auto durata = cautaCamp(campuri, CampEdu::Durata)) {
        lectie.durata = convertesteIntregNenegativ(*durata, "Durata lectiei");
    }
    if (const auto codec = cautaCamp(campuri, CampEdu::Codec)) {
        lectie.codec = *codec;
    }
    if ((lectie.tip == TipLectieEdu::Text &&
         (!lectie.numarCuvinte || lectie.durata || lectie.codec)) ||
        (lectie.tip == TipLectieEdu::Video &&
         (lectie.numarCuvinte || !lectie.durata || !lectie.codec || lectie.codec->empty()))) {
        throw ExceptieEdu("Specializarea lectiei este invalida.");
    }
    return lectie;
}

std::string ProtocolEdu::codificaEvaluare(const EvaluarePublicEdu& e) {
    std::vector<CampProtocolEdu> campuri{
        {static_cast<std::uint16_t>(CampEdu::EvaluareId), std::to_string(e.id)},
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(e.cursId)},
        {static_cast<std::uint16_t>(CampEdu::ProprietarId), std::to_string(e.profesorId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), e.nume},
        {static_cast<std::uint16_t>(CampEdu::TipEvaluare), std::to_string(static_cast<unsigned>(e.tip))},
        {static_cast<std::uint16_t>(CampEdu::LimitaTimp), std::to_string(e.limitaTimp)},
        {static_cast<std::uint16_t>(CampEdu::EsteObligatorie), e.esteObligatorie ? "1" : "0"}};
    if (e.numarIntrebari) campuri.push_back({static_cast<std::uint16_t>(CampEdu::NumarIntrebari), std::to_string(*e.numarIntrebari)});
    if (e.pondere) campuri.push_back({static_cast<std::uint16_t>(CampEdu::Pondere), std::to_string(*e.pondere)});
    std::string rezultat;
    adaugaCampuri(rezultat, campuri);
    return rezultat;
}

EvaluarePublicEdu ProtocolEdu::decodificaEvaluare(const std::string& date) {
    std::size_t pozitie = 0;
    const auto c = citesteCampuri(date, pozitie);
    if (pozitie != date.size()) throw ExceptieEdu("Datele evaluarii sunt invalide.");
    verificaCampuriExacte(c, {CampEdu::EvaluareId, CampEdu::CursId, CampEdu::ProprietarId,
        CampEdu::Nume, CampEdu::TipEvaluare, CampEdu::LimitaTimp,
        CampEdu::EsteObligatorie, CampEdu::NumarIntrebari, CampEdu::Pondere}, "evaluarii");
    const auto id=cautaCamp(c,CampEdu::EvaluareId), curs=cautaCamp(c,CampEdu::CursId),
        profesor=cautaCamp(c,CampEdu::ProprietarId), nume=cautaCamp(c,CampEdu::Nume),
        tip=cautaCamp(c,CampEdu::TipEvaluare), limita=cautaCamp(c,CampEdu::LimitaTimp),
        obligatorie=cautaCamp(c,CampEdu::EsteObligatorie);
    if (!id||!curs||!profesor||!nume||!tip||!limita||!obligatorie) throw ExceptieEdu("Datele evaluarii sunt incomplete.");
    EvaluarePublicEdu e;
    e.id=convertesteIntregPozitiv(*id,"Id-ul evaluarii");
    e.cursId=convertesteIntregPozitiv(*curs,"Id-ul cursului");
    e.profesorId=convertesteIntregPozitiv(*profesor,"Id-ul profesorului"); e.nume=*nume;
    const auto tv=convertesteIntregNenegativ(*tip,"Tipul evaluarii");
    if (tv<1||tv>2) throw ExceptieEdu("Tipul evaluarii este invalid.");
    e.tip=static_cast<TipEvaluareEdu>(tv);
    e.limitaTimp=convertesteIntregNenegativ(*limita,"Limita de timp");
    if (*obligatorie!="0"&&*obligatorie!="1") throw ExceptieEdu("Valoarea obligatorie este invalida.");
    e.esteObligatorie=*obligatorie=="1";
    if (auto v=cautaCamp(c,CampEdu::NumarIntrebari)) e.numarIntrebari=convertesteIntregNenegativ(*v,"Numarul de intrebari");
    if (auto v=cautaCamp(c,CampEdu::Pondere)) e.pondere=convertesteRealNenegativ(*v,"Ponderea");
    return e;
}

std::string ProtocolEdu::codificaIntrebare(const IntrebarePublicEdu& i) {
    std::string rezultat;
    adaugaCampuri(rezultat, {{static_cast<std::uint16_t>(CampEdu::IntrebareId),std::to_string(i.id)},
        {static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(i.evaluareId)},
        {static_cast<std::uint16_t>(CampEdu::Enunt),i.enunt},
        {static_cast<std::uint16_t>(CampEdu::PunctajMaxim),std::to_string(i.punctajMaxim)},
        {static_cast<std::uint16_t>(CampEdu::Ordine),std::to_string(i.ordine)}});
    return rezultat;
}

IntrebarePublicEdu ProtocolEdu::decodificaIntrebare(const std::string& date) {
    std::size_t pozitie=0; const auto c=citesteCampuri(date,pozitie);
    if (pozitie!=date.size()) throw ExceptieEdu("Datele intrebarii sunt invalide.");
    verificaCampuriExacte(c,{CampEdu::IntrebareId,CampEdu::EvaluareId,CampEdu::Enunt,CampEdu::PunctajMaxim,CampEdu::Ordine},"intrebarii");
    const auto id=cautaCamp(c,CampEdu::IntrebareId), ev=cautaCamp(c,CampEdu::EvaluareId), en=cautaCamp(c,CampEdu::Enunt), p=cautaCamp(c,CampEdu::PunctajMaxim), o=cautaCamp(c,CampEdu::Ordine);
    if(!id||!ev||!en||!p||!o) throw ExceptieEdu("Datele intrebarii sunt incomplete.");
    return {convertesteIntregPozitiv(*id,"Id-ul intrebarii"),convertesteIntregPozitiv(*ev,"Id-ul evaluarii"),*en,convertesteRealNenegativ(*p,"Punctajul"),convertesteIntregNenegativ(*o,"Ordinea")};
}
