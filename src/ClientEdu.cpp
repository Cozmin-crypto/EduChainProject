#include "ClientEdu.h"

#include "ExceptieEdu.h"

#include <WS2tcpip.h>

#include <limits>
#include <utility>

namespace {
std::string eroareClient(const std::string& context) {
    return context + " (cod Winsock " + std::to_string(WSAGetLastError()) + ").";
}

int convertesteId(const std::string& valoare, const std::string& denumire) {
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

ClientEdu::ClientEdu(std::uint16_t portNou, std::string adresaIpNoua)
    : ManagerSocket(std::move(adresaIpNoua), portNou) {
}

ClientEdu::~ClientEdu() noexcept {
    opresteNod();
}

void ClientEdu::pornesteNod() {
    if (conectat) {
        throw ExceptieEdu("Clientul este deja conectat.");
    }
    if (port == 0) {
        throw ExceptieEdu("Portul serverului trebuie sa fie strict pozitiv.");
    }
    SocketWindows socketNou(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (!socketNou.esteValid()) {
        throw ExceptieEdu(eroareClient("Crearea socket-ului clientului a esuat"));
    }
    sockaddr_in adresaServer{};
    adresaServer.sin_family = AF_INET;
    adresaServer.sin_port = htons(port);
    if (InetPtonA(AF_INET, adresaIp.c_str(), &adresaServer.sin_addr) != 1) {
        throw ExceptieEdu("Adresa IPv4 a serverului este invalida.");
    }
    if (connect(socketNou.obtine(), reinterpret_cast<const sockaddr*>(&adresaServer),
                sizeof(adresaServer)) == SOCKET_ERROR) {
        throw ExceptieEdu(eroareClient("Conectarea la server a esuat"));
    }
    socketPrincipal = std::move(socketNou);
    conectat = true;
    autentificat = false;
    rolAutentificat.clear();
}

void ClientEdu::opresteNod() {
    conectat = false;
    autentificat = false;
    rolAutentificat.clear();
    if (socketPrincipal.esteValid()) {
        shutdown(socketPrincipal.obtine(), SD_BOTH);
        socketPrincipal.inchide();
    }
}

std::uint32_t ClientEdu::genereazaIdCerere() {
    if (urmatorulIdCerere == 0) {
        urmatorulIdCerere = 1;
    }
    return urmatorulIdCerere++;
}

RaspunsEdu ClientEdu::executaCerere(CerereEdu cerere) {
    if (!conectat || !socketPrincipal.esteValid()) {
        throw ExceptieEdu("Clientul nu este conectat la server.");
    }
    if (cerere.idCerere == 0) {
        cerere.idCerere = genereazaIdCerere();
    }
    try {
        trimiteMesaj(socketPrincipal.obtine(), ProtocolEdu::codificaCerere(cerere));
        const auto cadru = primesteMesaj(socketPrincipal.obtine());
        if (!cadru.has_value()) {
            opresteNod();
            throw ExceptieEdu("Serverul a inchis conexiunea fara raspuns.");
        }
        RaspunsEdu raspuns = ProtocolEdu::decodificaRaspuns(*cadru);
        if (raspuns.idCerere != cerere.idCerere) {
            throw ExceptieEdu("ID-ul raspunsului nu corespunde cererii.");
        }
        ultimulRaspuns = raspuns.mesajPublic;
        return raspuns;
    } catch (...) {
        if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAESHUTDOWN ||
            WSAGetLastError() == WSAENOTCONN) {
            opresteNod();
        }
        throw;
    }
}

void ClientEdu::trimiteCerere() {
    ultimulRaspuns = trimiteCerere("PING");
}

std::string ClientEdu::trimiteCerere(const std::string& mesaj) {
    CerereEdu cerere;
    cerere.tip = mesaj == "PING"
        ? TipCerereEdu::Ping
        : static_cast<TipCerereEdu>(std::numeric_limits<std::uint16_t>::max());
    const auto raspuns = executaCerere(std::move(cerere));
    return raspuns.mesajPublic;
}

RaspunsEdu ClientEdu::autentifica(const std::string& email,
                                  const std::string& parola) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::Autentificare;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::Email), email},
        {static_cast<std::uint16_t>(CampEdu::Parola), parola}};
    RaspunsEdu raspuns = executaCerere(std::move(cerere));
    if (raspuns.cod == CodRezultatEdu::Succes) {
        const auto rol = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Rol);
        const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::UtilizatorId);
        if (!rol || !id) {
            throw ExceptieEdu("Raspunsul autentificarii este incomplet.");
        }
        convertesteId(*id, "Id-ul utilizatorului autentificat");
        autentificat = true;
        rolAutentificat = *rol;
    }
    return raspuns;
}

int ClientEdu::inregistreazaStudent(const std::string& n,const std::string& p,const std::string& e,const std::string& parola){CerereEdu c;c.tip=TipCerereEdu::InregistrareStudent;c.campuri={{static_cast<std::uint16_t>(CampEdu::Nume),n},{static_cast<std::uint16_t>(CampEdu::Prenume),p},{static_cast<std::uint16_t>(CampEdu::Email),e},{static_cast<std::uint16_t>(CampEdu::Parola),parola}};const auto r=executaCerere(std::move(c));verificaSucces(r);const auto id=ProtocolEdu::cautaCamp(r.campuri,CampEdu::UtilizatorId);if(!id)throw ExceptieEdu("Raspunsul inregistrarii este incomplet.");return convertesteId(*id,"Id-ul utilizatorului");}

void ClientEdu::verificaSucces(const RaspunsEdu& raspuns) {
    if (raspuns.cod != CodRezultatEdu::Succes) {
        throw ExceptieEdu(raspuns.mesajPublic);
    }
}

std::vector<CursPublicEdu> ClientEdu::listeazaCursuri() {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ListeazaCursuri;
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    std::vector<CursPublicEdu> cursuri;
    for (const auto& camp : raspuns.campuri) {
        if (camp.id != static_cast<std::uint16_t>(CampEdu::Curs)) {
            throw ExceptieEdu("Raspunsul listei de cursuri contine un camp invalid.");
        }
        cursuri.push_back(ProtocolEdu::decodificaCurs(camp.valoare));
    }
    return cursuri;
}

std::optional<CursPublicEdu> ClientEdu::obtineCurs(int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ObtineCurs;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                       std::to_string(cursId)}};
    const auto raspuns = executaCerere(std::move(cerere));
    if (raspuns.cod == CodRezultatEdu::ResursaInexistenta) {
        return std::nullopt;
    }
    verificaSucces(raspuns);
    const auto curs = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Curs);
    if (!curs) {
        throw ExceptieEdu("Raspunsul nu contine cursul solicitat.");
    }
    return ProtocolEdu::decodificaCurs(*curs);
}

int ClientEdu::creeazaCurs(const std::string& nume,
                           std::optional<int> parinteId,
                           std::optional<int> proprietarId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::CreeazaCurs;
    cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Nume), nume});
    if (parinteId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ParinteId),
                                  std::to_string(*parinteId)});
    }
    if (proprietarId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ProprietarId),
                                  std::to_string(*proprietarId)});
    }
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::CursId);
    if (!id) {
        throw ExceptieEdu("Raspunsul nu contine ID-ul cursului creat.");
    }
    return convertesteId(*id, "Id-ul cursului creat");
}

void ClientEdu::actualizeazaCurs(int cursId,
                                 const std::string& nume,
                                 std::optional<int> parinteId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ActualizeazaCurs;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), nume}};
    if (parinteId) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::ParinteId),
                                  std::to_string(*parinteId)});
    }
    verificaSucces(executaCerere(std::move(cerere)));
}

void ClientEdu::stergeCurs(int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::StergeCurs;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                       std::to_string(cursId)}};
    verificaSucces(executaCerere(std::move(cerere)));
}

std::vector<LectiePublicEdu> ClientEdu::listeazaLectii(int cursId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ListeazaLectii;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::CursId),
                       std::to_string(cursId)}};
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    std::vector<LectiePublicEdu> lectii;
    for (const auto& camp : raspuns.campuri) {
        if (camp.id != static_cast<std::uint16_t>(CampEdu::Lectie)) {
            throw ExceptieEdu("Raspunsul listei de lectii contine un camp invalid.");
        }
        lectii.push_back(ProtocolEdu::decodificaLectie(camp.valoare));
    }
    return lectii;
}

std::optional<LectiePublicEdu> ClientEdu::obtineLectie(int lectieId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ObtineLectie;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::LectieId),
                       std::to_string(lectieId)}};
    const auto raspuns = executaCerere(std::move(cerere));
    if (raspuns.cod == CodRezultatEdu::ResursaInexistenta) {
        return std::nullopt;
    }
    verificaSucces(raspuns);
    const auto lectie = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::Lectie);
    if (!lectie) {
        throw ExceptieEdu("Raspunsul nu contine lectia solicitata.");
    }
    return ProtocolEdu::decodificaLectie(*lectie);
}

int ClientEdu::creeazaLectieText(int cursId,
                                 const std::string& nume,
                                 const std::string& continut,
                                 long long dimensiuneOcteti,
                                 long long numarCuvinte) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::CreeazaLectieText;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), nume},
        {static_cast<std::uint16_t>(CampEdu::Continut), continut},
        {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti),
         std::to_string(dimensiuneOcteti)},
        {static_cast<std::uint16_t>(CampEdu::NumarCuvinte),
         std::to_string(numarCuvinte)}};
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::LectieId);
    if (!id) {
        throw ExceptieEdu("Raspunsul nu contine ID-ul lectiei create.");
    }
    return convertesteId(*id, "Id-ul lectiei create");
}

int ClientEdu::creeazaLectieVideo(int cursId,
                                  const std::string& nume,
                                  const std::string& continut,
                                  long long dimensiuneOcteti,
                                  long long durata,
                                  const std::string& codec) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::CreeazaLectieVideo;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), nume},
        {static_cast<std::uint16_t>(CampEdu::Continut), continut},
        {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti),
         std::to_string(dimensiuneOcteti)},
        {static_cast<std::uint16_t>(CampEdu::Durata), std::to_string(durata)},
        {static_cast<std::uint16_t>(CampEdu::Codec), codec}};
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    const auto id = ProtocolEdu::cautaCamp(raspuns.campuri, CampEdu::LectieId);
    if (!id) {
        throw ExceptieEdu("Raspunsul nu contine ID-ul lectiei create.");
    }
    return convertesteId(*id, "Id-ul lectiei create");
}

void ClientEdu::actualizeazaLectie(const CerereActualizareLectieClient& date) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::ActualizeazaLectie;
    cerere.campuri = {
        {static_cast<std::uint16_t>(CampEdu::LectieId), std::to_string(date.lectieId)},
        {static_cast<std::uint16_t>(CampEdu::CursId), std::to_string(date.cursId)},
        {static_cast<std::uint16_t>(CampEdu::Nume), date.nume},
        {static_cast<std::uint16_t>(CampEdu::TipLectie),
         std::to_string(static_cast<std::uint16_t>(date.tip))},
        {static_cast<std::uint16_t>(CampEdu::Continut), date.continut},
        {static_cast<std::uint16_t>(CampEdu::DimensiuneOcteti),
         std::to_string(date.dimensiuneOcteti)}};
    if (date.numarCuvinte) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::NumarCuvinte),
                                  std::to_string(*date.numarCuvinte)});
    }
    if (date.durata) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Durata),
                                  std::to_string(*date.durata)});
    }
    if (date.codec) {
        cerere.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Codec), *date.codec});
    }
    verificaSucces(executaCerere(std::move(cerere)));
}

void ClientEdu::stergeLectie(int lectieId) {
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::StergeLectie;
    cerere.campuri = {{static_cast<std::uint16_t>(CampEdu::LectieId),
                       std::to_string(lectieId)}};
    verificaSucces(executaCerere(std::move(cerere)));
}

std::vector<EvaluarePublicEdu> ClientEdu::listeazaEvaluari(int cursId) {
    CerereEdu c; c.tip=TipCerereEdu::ListeazaEvaluari;
    c.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(cursId)}};
    const auto r=executaCerere(std::move(c)); verificaSucces(r);
    std::vector<EvaluarePublicEdu> rezultat;
    for(const auto& camp:r.campuri){ if(camp.id!=static_cast<std::uint16_t>(CampEdu::Evaluare)) throw ExceptieEdu("Raspunsul contine un camp neasteptat."); rezultat.push_back(ProtocolEdu::decodificaEvaluare(camp.valoare)); }
    return rezultat;
}

std::optional<EvaluarePublicEdu> ClientEdu::obtineEvaluare(int id) {
    CerereEdu c; c.tip=TipCerereEdu::ObtineEvaluare; c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(id)}};
    const auto r=executaCerere(std::move(c)); if(r.cod==CodRezultatEdu::ResursaInexistenta)return std::nullopt; verificaSucces(r);
    if(r.campuri.size()!=1||r.campuri[0].id!=static_cast<std::uint16_t>(CampEdu::Evaluare))throw ExceptieEdu("Raspunsul evaluarii este invalid.");
    return ProtocolEdu::decodificaEvaluare(r.campuri[0].valoare);
}

namespace {
void adaugaCampuriEvaluareClient(CerereEdu& c,const CerereSalvareEvaluareClient& d){
    c.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(d.cursId)},{static_cast<std::uint16_t>(CampEdu::Nume),d.nume},{static_cast<std::uint16_t>(CampEdu::LimitaTimp),std::to_string(d.limitaTimp)},{static_cast<std::uint16_t>(CampEdu::EsteObligatorie),d.esteObligatorie?"1":"0"}};
    if(d.numarIntrebari)c.campuri.push_back({static_cast<std::uint16_t>(CampEdu::NumarIntrebari),std::to_string(*d.numarIntrebari)});
    if(d.pondere)c.campuri.push_back({static_cast<std::uint16_t>(CampEdu::Pondere),std::to_string(*d.pondere)});
}
int idCreatEvaluareClient(ClientEdu& client,CerereEdu c){const auto r=client.executaCerere(std::move(c)); if(r.cod!=CodRezultatEdu::Succes)throw ExceptieEdu(r.mesajPublic); const auto id=ProtocolEdu::cautaCamp(r.campuri,CampEdu::EvaluareId);if(!id)throw ExceptieEdu("Raspuns incomplet.");return convertesteId(*id,"Id-ul evaluarii");}
}
int ClientEdu::creeazaChestionar(const CerereSalvareEvaluareClient& d){CerereEdu c;c.tip=TipCerereEdu::CreeazaChestionar;adaugaCampuriEvaluareClient(c,d);return idCreatEvaluareClient(*this,std::move(c));}
int ClientEdu::creeazaExamenFinal(const CerereSalvareEvaluareClient& d){CerereEdu c;c.tip=TipCerereEdu::CreeazaExamenFinal;adaugaCampuriEvaluareClient(c,d);return idCreatEvaluareClient(*this,std::move(c));}
void ClientEdu::actualizeazaEvaluare(const CerereActualizareEvaluareClient& d){CerereEdu c;c.tip=TipCerereEdu::ActualizeazaEvaluare;adaugaCampuriEvaluareClient(c,d);c.campuri.push_back({static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(d.evaluareId)});c.campuri.push_back({static_cast<std::uint16_t>(CampEdu::TipEvaluare),std::to_string(static_cast<unsigned>(d.tip))});verificaSucces(executaCerere(std::move(c)));}
void ClientEdu::stergeEvaluare(int id){CerereEdu c;c.tip=TipCerereEdu::StergeEvaluare;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(id)}};verificaSucces(executaCerere(std::move(c)));}
std::vector<IntrebarePublicEdu> ClientEdu::listeazaIntrebari(int id){CerereEdu c;c.tip=TipCerereEdu::ListeazaIntrebari;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(id)}};const auto r=executaCerere(std::move(c));verificaSucces(r);std::vector<IntrebarePublicEdu> v;for(const auto& x:r.campuri){if(x.id!=static_cast<std::uint16_t>(CampEdu::Intrebare))throw ExceptieEdu("Raspuns invalid.");v.push_back(ProtocolEdu::decodificaIntrebare(x.valoare));}return v;}
int ClientEdu::adaugaIntrebare(int ev,const std::string& en,const std::string& corect,double p,long long o){CerereEdu c;c.tip=TipCerereEdu::AdaugaIntrebare;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(ev)},{static_cast<std::uint16_t>(CampEdu::Enunt),en},{static_cast<std::uint16_t>(CampEdu::RaspunsCorect),corect},{static_cast<std::uint16_t>(CampEdu::PunctajMaxim),std::to_string(p)},{static_cast<std::uint16_t>(CampEdu::Ordine),std::to_string(o)}};const auto r=executaCerere(std::move(c));verificaSucces(r);const auto id=ProtocolEdu::cautaCamp(r.campuri,CampEdu::IntrebareId);if(!id)throw ExceptieEdu("Raspuns incomplet.");return convertesteId(*id,"Id-ul intrebarii");}
void ClientEdu::actualizeazaIntrebare(int ev,int id,const std::string& en,const std::string& corect,double p,long long o){CerereEdu c;c.tip=TipCerereEdu::ActualizeazaIntrebare;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(ev)},{static_cast<std::uint16_t>(CampEdu::IntrebareId),std::to_string(id)},{static_cast<std::uint16_t>(CampEdu::Enunt),en},{static_cast<std::uint16_t>(CampEdu::RaspunsCorect),corect},{static_cast<std::uint16_t>(CampEdu::PunctajMaxim),std::to_string(p)},{static_cast<std::uint16_t>(CampEdu::Ordine),std::to_string(o)}};verificaSucces(executaCerere(std::move(c)));}
void ClientEdu::stergeIntrebare(int ev,int id){CerereEdu c;c.tip=TipCerereEdu::StergeIntrebare;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(ev)},{static_cast<std::uint16_t>(CampEdu::IntrebareId),std::to_string(id)}};verificaSucces(executaCerere(std::move(c)));}

int ClientEdu::pornesteIncercare(int evaluareId){CerereEdu c;c.tip=TipCerereEdu::PornesteIncercare;c.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(evaluareId)}};const auto r=executaCerere(std::move(c));verificaSucces(r);const auto id=ProtocolEdu::cautaCamp(r.campuri,CampEdu::IncercareId);if(!id)throw ExceptieEdu("Raspunsul incercarii este incomplet.");return convertesteId(*id,"Id-ul incercarii");}

std::optional<IncercarePublicEdu> ClientEdu::obtineIncercare(int incercareId){CerereEdu c;c.tip=TipCerereEdu::ObtineIncercare;c.campuri={{static_cast<std::uint16_t>(CampEdu::IncercareId),std::to_string(incercareId)}};const auto r=executaCerere(std::move(c));if(r.cod==CodRezultatEdu::ResursaInexistenta)return std::nullopt;verificaSucces(r);if(r.campuri.size()!=1||r.campuri[0].id!=static_cast<std::uint16_t>(CampEdu::Incercare))throw ExceptieEdu("Raspunsul incercarii este invalid.");return ProtocolEdu::decodificaIncercare(r.campuri[0].valoare);}

void ClientEdu::salveazaRaspuns(int incercareId,int intrebareId,const std::string& continut){CerereEdu c;c.tip=TipCerereEdu::SalveazaRaspuns;c.campuri={{static_cast<std::uint16_t>(CampEdu::IncercareId),std::to_string(incercareId)},{static_cast<std::uint16_t>(CampEdu::IntrebareId),std::to_string(intrebareId)},{static_cast<std::uint16_t>(CampEdu::Raspuns),continut}};verificaSucces(executaCerere(std::move(c)));}

IncercarePublicEdu ClientEdu::finalizeazaIncercare(int incercareId){CerereEdu c;c.tip=TipCerereEdu::FinalizeazaIncercare;c.campuri={{static_cast<std::uint16_t>(CampEdu::IncercareId),std::to_string(incercareId)}};const auto r=executaCerere(std::move(c));verificaSucces(r);if(r.campuri.size()!=1||r.campuri[0].id!=static_cast<std::uint16_t>(CampEdu::Incercare))throw ExceptieEdu("Rezultatul finalizarii este invalid.");return ProtocolEdu::decodificaIncercare(r.campuri[0].valoare);}

namespace {void executaInscriere(ClientEdu& client,TipCerereEdu tip,int student,int curs){CerereEdu c;c.tip=tip;c.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(curs)}};if(student>0)c.campuri.push_back({static_cast<std::uint16_t>(CampEdu::StudentId),std::to_string(student)});const auto r=client.executaCerere(std::move(c));if(r.cod!=CodRezultatEdu::Succes)throw ExceptieEdu(r.mesajPublic);}}
void ClientEdu::inscrieLaCurs(int c){executaInscriere(*this,TipCerereEdu::InscrieStudentLaCurs,0,c);}void ClientEdu::retrageDeLaCurs(int c){executaInscriere(*this,TipCerereEdu::RetrageStudentDeLaCurs,0,c);}void ClientEdu::inscrieStudentLaCurs(int s,int c){executaInscriere(*this,TipCerereEdu::InscrieStudentLaCurs,s,c);}void ClientEdu::retrageStudentDeLaCurs(int s,int c){executaInscriere(*this,TipCerereEdu::RetrageStudentDeLaCurs,s,c);}
std::vector<CursPublicEdu> ClientEdu::listeazaCursuriInscrise(){CerereEdu c;c.tip=TipCerereEdu::ListeazaCursuriInscrise;const auto r=executaCerere(std::move(c));verificaSucces(r);std::vector<CursPublicEdu> v;for(const auto&x:r.campuri){if(x.id!=static_cast<std::uint16_t>(CampEdu::Curs))throw ExceptieEdu("Raspuns invalid.");v.push_back(ProtocolEdu::decodificaCurs(x.valoare));}return v;}
std::vector<StudentPublicEdu> ClientEdu::listeazaStudentiCurs(int curs){CerereEdu c;c.tip=TipCerereEdu::ListeazaStudentiCurs;c.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(curs)}};const auto r=executaCerere(std::move(c));verificaSucces(r);std::vector<StudentPublicEdu> v;for(const auto&x:r.campuri){if(x.id!=static_cast<std::uint16_t>(CampEdu::StudentPublic))throw ExceptieEdu("Raspuns invalid.");v.push_back(ProtocolEdu::decodificaStudent(x.valoare));}return v;}
bool ClientEdu::verificaInscriere(int student,int curs){CerereEdu c;c.tip=TipCerereEdu::VerificaInscriere;c.campuri={{static_cast<std::uint16_t>(CampEdu::StudentId),std::to_string(student)},{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(curs)}};const auto r=executaCerere(std::move(c));verificaSucces(r);const auto x=ProtocolEdu::cautaCamp(r.campuri,CampEdu::Inscris);if(!x||(*x!="0"&&*x!="1"))throw ExceptieEdu("Raspuns inscriere invalid.");return *x=="1";}

void ClientEdu::deconecteaza() {
    if (!conectat) {
        return;
    }
    CerereEdu cerere;
    cerere.tip = TipCerereEdu::Deconectare;
    const auto raspuns = executaCerere(std::move(cerere));
    verificaSucces(raspuns);
    opresteNod();
}

bool ClientEdu::esteConectat() const noexcept {
    return conectat;
}

bool ClientEdu::esteAutentificat() const noexcept {
    return autentificat;
}

const std::string& ClientEdu::obtineRolAutentificat() const noexcept {
    return rolAutentificat;
}

const std::string& ClientEdu::obtineUltimulRaspuns() const noexcept {
    return ultimulRaspuns;
}
