#include "AutentificareService.h"

#include "ExceptieEdu.h"
#include "UtilizatorRepository.h"

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
    if (utilizator->parola != parola) {
        return {false, StareAutentificare::ParolaIncorecta, std::nullopt,
                email, std::nullopt, mesajEsec};
    }
    return {true, StareAutentificare::Succes, utilizator->id,
            utilizator->email, utilizator->rol, "Autentificare reusita."};
}
