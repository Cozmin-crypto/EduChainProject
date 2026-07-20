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

int AutentificareService::inregistreaza(const std::string& n,const std::string& p,const std::string& e,const std::string& parola,const std::string& rol){
 if(n.empty()||p.empty())throw ExceptieEdu("Numele si prenumele sunt obligatorii."); if(e.empty()||e.find('@')==std::string::npos||e.find('.')==std::string::npos)throw ExceptieEdu("Email invalid."); if(parola.size()<6)throw ExceptieEdu("Parola trebuie sa aiba cel putin 6 caractere."); if(rol!="student"&&rol!="profesor")throw ExceptieEdu("Rolul trebuie sa fie student sau profesor."); if(utilizatori.cautaDupaEmail(e))throw ExceptieEdu("Email deja utilizat."); return utilizatori.inregistreazaUtilizator(e,parola,rol); }

int AutentificareService::inregistreazaStudent(const std::string& n,const std::string& p,const std::string& e,const std::string& parola){
 return inregistreaza(n,p,e,parola,"student"); }
