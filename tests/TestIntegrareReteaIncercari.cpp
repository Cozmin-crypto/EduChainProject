#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "EvaluareRepository.h"
#include "EvaluareService.h"
#include "ExceptieEdu.h"
#include "LectieRepository.h"
#include "LectieService.h"
#include "ManagerSocket.h"
#include "ServerEdu.h"
#include "UtilizatorRepository.h"
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <thread>
namespace {
void verifica(bool c,const char* m){if(!c)throw std::runtime_error(m);}
bool esueaza(const std::function<void()>& f){try{f();}catch(const ExceptieEdu&){return true;}return false;}
void profesor(ConectorBazaDate& d,int id){d.executaInterogareParametrizata("INSERT INTO personal (utilizator_id, departament, data_angajarii) VALUES (?, ?, ?);",{std::to_string(id),"IT","2026-07-19"});d.executaInterogareParametrizata("INSERT INTO profesori (utilizator_id) VALUES (?);",{std::to_string(id)});}
void student(ConectorBazaDate& d,int id){d.executaInterogareParametrizata("INSERT INTO studenti (utilizator_id) VALUES (?);",{std::to_string(id)});}
template<class F>void conexiune(ServerEdu& s,F f){std::exception_ptr e;std::thread t([&]{try{s.proceseazaCerere();}catch(...){e=std::current_exception();}});try{ClientEdu c(s.obtinePort());c.pornesteNod();f(c);if(c.esteConectat())c.deconecteaza();}catch(...){s.opresteNod();t.join();throw;}t.join();if(e)std::rethrow_exception(e);}
}
int main(){
 const auto cale=std::filesystem::temp_directory_path()/("educhain_incercari_retea_"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count())+".db");
 const int socketuri=SocketWindows::numarSocketuriActive(),winsock=InitializatorWinsock::numarInstanteActive();ConectorBazaDate db;
 try{db.deschideConexiune(cale.string());UtilizatorRepository ur(db);CursRepository cr(db);LectieRepository lr(db);EvaluareRepository er(db);AutentificareService auth(ur);CursService cs(cr,ur);LectieService ls(lr,cr,ur);EvaluareService es(er,cr,ur);
  const int prof=ur.adaugaUtilizator("p@i.ro","p","profesor"),s1=ur.adaugaUtilizator("s1@i.ro","s1","student"),s2=ur.adaugaUtilizator("s2@i.ro","s2","student");profesor(db,prof);student(db,s1);student(db,s2);
  const int curs=cs.creeazaCurs({prof,prof,"Curs",std::nullopt});const int evaluare=es.creeazaEvaluare({prof,curs,"Test","chestionar",30,true,2,std::nullopt});
  const int q1=es.adaugaIntrebare({prof,evaluare,"Capitala Romaniei?","Bucuresti",1,0});const int q2=es.adaugaIntrebare({prof,evaluare,"2 + 2?","4",2,1});
  const int evaluareZero=es.creeazaEvaluare({prof,curs,"Test zero","chestionar",30,true,1,std::nullopt});es.adaugaIntrebare({prof,evaluareZero,"Fara punctaj","x",0,0});
  {ServerEdu server(auth,cs,ls,es,0);server.pornesteNod();int incercare=0;
   conexiune(server,[&](ClientEdu& c){CerereEdu r;r.tip=TipCerereEdu::PornesteIncercare;r.campuri={{static_cast<std::uint16_t>(CampEdu::EvaluareId),std::to_string(evaluare)}};verifica(c.executaCerere(r).cod==CodRezultatEdu::AccesInterzis,"fara autentificare");});
   conexiune(server,[&](ClientEdu& c){c.autentifica("s1@i.ro","s1");incercare=c.pornesteIncercare(evaluare);CerereEdu injectiePunctaj;injectiePunctaj.tip=TipCerereEdu::SalveazaRaspuns;injectiePunctaj.campuri={{static_cast<std::uint16_t>(CampEdu::IncercareId),std::to_string(incercare)},{static_cast<std::uint16_t>(CampEdu::IntrebareId),std::to_string(q1)},{static_cast<std::uint16_t>(CampEdu::Raspuns),"Bucuresti"},{static_cast<std::uint16_t>(CampEdu::PunctajObtinut),"999"}};verifica(c.executaCerere(injectiePunctaj).cod==CodRezultatEdu::ValidareEsuata,"injectie punctaj");c.salveazaRaspuns(incercare,q1,"  bUcUrEsTi \t");c.salveazaRaspuns(incercare,q2,"gresit");const auto rezultat=c.finalizeazaIncercare(incercare);verifica(rezultat.scorBrut==1&&rezultat.notaFinala==3.33,"calcul sau rotunjire incorecta");verifica(esueaza([&]{c.salveazaRaspuns(incercare,q2,"4");}),"raspuns dupa finalizare");verifica(esueaza([&]{c.finalizeazaIncercare(incercare);}),"finalizare repetata");const int incercareZero=c.pornesteIncercare(evaluareZero);verifica(esueaza([&]{c.finalizeazaIncercare(incercareZero);}),"punctaj maxim zero");CerereEdu injectie;injectie.tip=TipCerereEdu::FinalizeazaIncercare;injectie.campuri={{static_cast<std::uint16_t>(CampEdu::IncercareId),std::to_string(incercare)},{static_cast<std::uint16_t>(CampEdu::NotaFinala),"10"}};verifica(c.executaCerere(injectie).cod==CodRezultatEdu::ValidareEsuata,"injectie nota");verifica(c.trimiteCerere("PING")=="PONG","conexiune dupa eroare");});
   conexiune(server,[&](ClientEdu& c){c.autentifica("s2@i.ro","s2");verifica(esueaza([&]{c.obtineIncercare(incercare);}),"incercarea altui student");});server.opresteNod();}
  const auto publice=es.listeazaIntrebari(evaluare);verifica(publice.front().raspunsCorect=="Bucuresti","raspuns intern indisponibil");db.inchideConexiune();std::filesystem::remove(cale);verifica(SocketWindows::numarSocketuriActive()==socketuri,"socket activ");verifica(InitializatorWinsock::numarInstanteActive()==winsock,"winsock activ");std::cout<<"Test integrare retea incercari: SUCCES\n";return 0;
 }catch(const std::exception& e){db.inchideConexiune();std::filesystem::remove(cale);std::cerr<<e.what()<<'\n';return 1;}
}
