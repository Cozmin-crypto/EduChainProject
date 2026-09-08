#include "AutentificareService.h"

#include "ExceptieEdu.h"
#include "UtilizatorRepository.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <bcrypt.h>

#include <array>
#include <charconv>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <vector>

namespace {
constexpr std::uint64_t iteratiiPbkdf2 = 600000;
constexpr std::size_t lungimeSalt = 16;
constexpr std::size_t lungimeCheie = 32;
constexpr std::string_view prefixHash = "$pbkdf2-sha256$";

class FurnizorHmac final {
public:
    FurnizorHmac() {
        if (BCryptOpenAlgorithmProvider(&handle_, BCRYPT_SHA256_ALGORITHM,
                                        nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG) < 0) {
            throw ExceptieEdu("Furnizorul criptografic nu este disponibil.");
        }
    }
    ~FurnizorHmac() {
        if (handle_ != nullptr) {
            BCryptCloseAlgorithmProvider(handle_, 0);
        }
    }
    FurnizorHmac(const FurnizorHmac&) = delete;
    FurnizorHmac& operator=(const FurnizorHmac&) = delete;
    BCRYPT_ALG_HANDLE get() const noexcept { return handle_; }

private:
    BCRYPT_ALG_HANDLE handle_{};
};

std::string codificaHex(const std::vector<unsigned char>& date) {
    std::ostringstream rezultat;
    rezultat << std::hex << std::setfill('0');
    for (const unsigned char octet : date) {
        rezultat << std::setw(2) << static_cast<unsigned int>(octet);
    }
    return rezultat.str();
}

bool decodificaHex(std::string_view text, std::vector<unsigned char>& rezultat) {
    if (text.empty() || text.size() % 2 != 0) {
        return false;
    }
    rezultat.clear();
    rezultat.reserve(text.size() / 2);
    for (std::size_t pozitie = 0; pozitie < text.size(); pozitie += 2) {
        unsigned int valoare{};
        const auto conversie = std::from_chars(
            text.data() + pozitie, text.data() + pozitie + 2, valoare, 16);
        if (conversie.ec != std::errc{} ||
            conversie.ptr != text.data() + pozitie + 2 || valoare > 255) {
            rezultat.clear();
            return false;
        }
        rezultat.push_back(static_cast<unsigned char>(valoare));
    }
    return true;
}

std::vector<unsigned char> derivaCheie(const std::string& parola,
                                      const std::vector<unsigned char>& salt,
                                      std::uint64_t iteratii) {
    if (parola.size() > static_cast<std::size_t>(std::numeric_limits<ULONG>::max()) ||
        salt.size() > static_cast<std::size_t>(std::numeric_limits<ULONG>::max())) {
        throw ExceptieEdu("Datele parolei sunt prea lungi.");
    }
    FurnizorHmac furnizor;
    std::vector<unsigned char> cheie(lungimeCheie);
    const NTSTATUS stare = BCryptDeriveKeyPBKDF2(
        furnizor.get(),
        reinterpret_cast<PUCHAR>(const_cast<char*>(parola.data())),
        static_cast<ULONG>(parola.size()),
        const_cast<PUCHAR>(salt.data()), static_cast<ULONG>(salt.size()),
        iteratii, cheie.data(), static_cast<ULONG>(cheie.size()), 0);
    if (stare < 0) {
        SecureZeroMemory(cheie.data(), cheie.size());
        throw ExceptieEdu("Derivarea sigura a parolei a esuat.");
    }
    return cheie;
}

std::string genereazaHash(const std::string& parola) {
    std::vector<unsigned char> salt(lungimeSalt);
    if (BCryptGenRandom(nullptr, salt.data(), static_cast<ULONG>(salt.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        throw ExceptieEdu("Generarea saltului parolei a esuat.");
    }
    auto cheie = derivaCheie(parola, salt, iteratiiPbkdf2);
    const std::string rezultat = std::string(prefixHash) +
        std::to_string(iteratiiPbkdf2) + "$" + codificaHex(salt) + "$" +
        codificaHex(cheie);
    SecureZeroMemory(cheie.data(), cheie.size());
    return rezultat;
}

bool verificaHash(const std::string& parola, const std::string& hashStocat) {
    if (hashStocat.rfind(prefixHash.data(), 0) != 0) {
        return false;
    }
    const std::size_t inceputIteratii = prefixHash.size();
    const std::size_t separatorSalt = hashStocat.find('$', inceputIteratii);
    const std::size_t separatorHash = separatorSalt == std::string::npos
        ? std::string::npos : hashStocat.find('$', separatorSalt + 1);
    if (separatorSalt == std::string::npos || separatorHash == std::string::npos ||
        hashStocat.find('$', separatorHash + 1) != std::string::npos) {
        return false;
    }
    std::uint64_t iteratii{};
    const auto textIteratii = std::string_view(hashStocat).substr(
        inceputIteratii, separatorSalt - inceputIteratii);
    const auto conversie = std::from_chars(
        textIteratii.data(), textIteratii.data() + textIteratii.size(), iteratii);
    if (conversie.ec != std::errc{} ||
        conversie.ptr != textIteratii.data() + textIteratii.size() ||
        iteratii < 100000 || iteratii > 5000000) {
        return false;
    }
    std::vector<unsigned char> salt;
    std::vector<unsigned char> asteptat;
    if (!decodificaHex(std::string_view(hashStocat).substr(
                           separatorSalt + 1, separatorHash - separatorSalt - 1), salt) ||
        !decodificaHex(std::string_view(hashStocat).substr(separatorHash + 1), asteptat) ||
        salt.size() < 16 || asteptat.size() != lungimeCheie) {
        return false;
    }
    auto calculat = derivaCheie(parola, salt, iteratii);
    unsigned char diferenta{};
    for (std::size_t index = 0; index < calculat.size(); ++index) {
        diferenta |= static_cast<unsigned char>(calculat[index] ^ asteptat[index]);
    }
    SecureZeroMemory(calculat.data(), calculat.size());
    return diferenta == 0;
}

bool esteHashPbkdf2(const std::string& valoare) {
    return valoare.rfind(prefixHash.data(), 0) == 0;
}
}

AutentificareService::AutentificareService(UtilizatorRepository& utilizatori)
    : utilizatori(utilizatori) {
}

RezultatAutentificare AutentificareService::autentifica(
    const std::string& email,
    const std::string& parola) {
    if (email.empty() || parola.empty()) {
        throw ExceptieEdu("Emailul si parola sunt obligatorii pentru autentificare.");
    }

    constexpr const char* mesajEsec = "Email sau parola incorecta.";
    const auto utilizator = utilizatori.cautaDupaEmail(email);
    if (!utilizator.has_value()) {
        return {false, StareAutentificare::EmailInexistent, std::nullopt,
                email, std::nullopt, mesajEsec};
    }
    bool parolaCorecta = false;
    if (esteHashPbkdf2(utilizator->parola)) {
        parolaCorecta = verificaHash(parola, utilizator->parola);
    } else {
        parolaCorecta = utilizator->parola == parola;
        if (parolaCorecta) {
            utilizatori.actualizeazaParola(utilizator->id, genereazaHash(parola));
        }
    }
    if (!parolaCorecta) {
        return {false, StareAutentificare::ParolaIncorecta, std::nullopt,
                email, std::nullopt, mesajEsec};
    }
    if (utilizator->rol != "student" && utilizator->rol != "profesor") {
        return {false, StareAutentificare::RolNesuportat, std::nullopt,
                email, std::nullopt, "Acces refuzat: rol nesuportat."};
    }
    if (utilizator->nume.empty() || utilizator->prenume.empty()) {
        throw ExceptieEdu("Contul nu contine un nume si un prenume valide.");
    }
    return {true, StareAutentificare::Succes, utilizator->id,
            utilizator->email, utilizator->rol, "Autentificare reusita.",
            utilizator->nume, utilizator->prenume};
}

int AutentificareService::inregistreaza(const std::string& n,const std::string& p,const std::string& e,const std::string& parola,const std::string& rol){
 if(n.empty()||p.empty())throw ExceptieEdu("Numele si prenumele sunt obligatorii."); if(e.empty()||e.find('@')==std::string::npos||e.find('.')==std::string::npos)throw ExceptieEdu("Email invalid."); if(parola.size()<6)throw ExceptieEdu("Parola trebuie sa aiba cel putin 6 caractere."); if(parola.size()>1024)throw ExceptieEdu("Parola este prea lunga."); if(rol!="student"&&rol!="profesor")throw ExceptieEdu("Rolul trebuie sa fie student sau profesor."); if(utilizatori.cautaDupaEmail(e))throw ExceptieEdu("Email deja utilizat."); return utilizatori.inregistreazaUtilizator(e,genereazaHash(parola),rol,n,p); }

int AutentificareService::inregistreazaStudent(const std::string& n,const std::string& p,const std::string& e,const std::string& parola){
 return inregistreaza(n,p,e,parola,"student"); }
