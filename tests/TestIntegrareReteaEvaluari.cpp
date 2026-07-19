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
void profesor(ConectorBazaDate& c,int id){c.executaInterogareParametrizata("INSERT INTO personal (utilizator_id, departament, data_angajarii) VALUES (?, ?, ?);",{std::to_string(id),"IT","2026-07-19"});c.executaInterogareParametrizata("INSERT INTO profesori (utilizator_id) VALUES (?);",{std::to_string(id)});}
void student(ConectorBazaDate& c,int id){c.executaInterogareParametrizata("INSERT INTO studenti (utilizator_id) VALUES (?);",{std::to_string(id)});}
void admin(ConectorBazaDate& c,int id){c.executaInterogareParametrizata("INSERT INTO personal (utilizator_id, departament, data_angajarii) VALUES (?, ?, ?);",{std::to_string(id),"Admin","2026-07-19"});c.executaInterogareParametrizata("INSERT INTO administratori (utilizator_id, nivel_acces) VALUES (?, ?);",{std::to_string(id),"10"});}
template<class F> void conexiune(ServerEdu& s,F f){std::exception_ptr e;std::thread t([&]{try{s.proceseazaCerere();}catch(...){e=std::current_exception();}});try{ClientEdu c(s.obtinePort());c.pornesteNod();f(c);if(c.esteConectat())c.deconecteaza();}catch(...){s.opresteNod();t.join();throw;}t.join();if(e)std::rethrow_exception(e);}
}

int main(){
 const auto cale=std::filesystem::temp_directory_path()/("educhain_evaluari_retea_"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count())+".db");
 const int sock=SocketWindows::numarSocketuriActive(),wsa=InitializatorWinsock::numarInstanteActive();
 ConectorBazaDate db;
 try{
  db.deschideConexiune(cale.string());UtilizatorRepository ur(db);CursRepository cr(db);LectieRepository lr(db);EvaluareRepository er(db);AutentificareService as(ur);CursService cs(cr,ur);LectieService ls(lr,cr,ur);EvaluareService es(er,cr,ur);
  int p1=ur.adaugaUtilizator("p1@edu.ro","p1","profesor"),p2=ur.adaugaUtilizator("p2@edu.ro","p2","profesor"),st=ur.adaugaUtilizator("s@edu.ro","s","student"),ad=ur.adaugaUtilizator("a@edu.ro","a","administrator");profesor(db,p1);profesor(db,p2);student(db,st);admin(db,ad);
  int curs=cs.creeazaCurs({p1,p1,"Curs evaluari",std::nullopt});
  {
  ServerEdu server(as,cs,ls,es,0);server.pornesteNod();
  conexiune(server,[&](ClientEdu& c){CerereEdu q;q.tip=TipCerereEdu::ListeazaEvaluari;q.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(curs)}};verifica(c.executaCerere(q).cod==CodRezultatEdu::AccesInterzis,"cererea fara login a fost permisa");});
  conexiune(server,[&](ClientEdu& c){c.autentifica("s@edu.ro","s");verifica(c.listeazaEvaluari(curs).empty(),"lista initiala");CerereSalvareEvaluareClient d{curs,"Interzis",10,true,2,std::nullopt};verifica(esueaza([&]{c.creeazaChestionar(d);}),"studentul a creat evaluare");verifica(c.trimiteCerere("PING")=="PONG","conexiunea nu a ramas activa");});
  conexiune(server,[&](ClientEdu& c){c.autentifica("p2@edu.ro","p2");CerereSalvareEvaluareClient d{curs,"Interzis",10,true,2,std::nullopt};verifica(esueaza([&]{c.creeazaChestionar(d);}),"profesorul neproprietar a creat evaluare");});
  int chestionar=0,examen=0,intrebare=0;
  conexiune(server,[&](ClientEdu& c){c.autentifica("p1@edu.ro","p1");CerereSalvareEvaluareClient ch{curs,"Chestionar ' SQL; --",30,true,3,std::nullopt};chestionar=c.creeazaChestionar(ch);CerereSalvareEvaluareClient ex{curs,"Examen",60,true,std::nullopt,0.5};examen=c.creeazaExamenFinal(ex);verifica(c.listeazaEvaluari(curs).size()==2,"listare evaluari");verifica(c.obtineEvaluare(chestionar)->tip==TipEvaluareEdu::Chestionar,"obtinere evaluare");verifica(!c.obtineEvaluare(999999),"evaluare inexistenta");intrebare=c.adaugaIntrebare(chestionar,"Cat face '1; DROP TABLE'?","literal",5.0,0);c.actualizeazaIntrebare(chestionar,intrebare,"Enunt actualizat","literal",7.5,1);auto v=c.listeazaIntrebari(chestionar);verifica(v.size()==1&&v[0].punctajMaxim==7.5,"CRUD intrebare");verifica(esueaza([&]{c.adaugaIntrebare(chestionar,"invalid","x",-1,2);}),"punctaj invalid");verifica(esueaza([&]{c.adaugaIntrebare(chestionar,"conflict","x",1,1);}),"conflict ordine");CerereActualizareEvaluareClient u;u.evaluareId=examen;u.cursId=curs;u.nume="Examen actualizat";u.tip=TipEvaluareEdu::ExamenFinal;u.limitaTimp=90;u.esteObligatorie=true;u.pondere=0.7;c.actualizeazaEvaluare(u);verifica(c.obtineEvaluare(examen)->nume=="Examen actualizat","actualizare evaluare");c.stergeIntrebare(chestionar,intrebare);verifica(c.listeazaIntrebari(chestionar).empty(),"stergere intrebare");});
  conexiune(server,[&](ClientEdu& c){c.autentifica("a@edu.ro","a");c.stergeEvaluare(examen);c.stergeEvaluare(chestionar);verifica(c.listeazaEvaluari(curs).empty(),"administrator/stergere");});
  server.opresteNod();
  }
  db.inchideConexiune();std::filesystem::remove(cale);verifica(SocketWindows::numarSocketuriActive()==sock,"socket ramas activ");verifica(InitializatorWinsock::numarInstanteActive()==wsa,"Winsock ramas activ");verifica(!std::filesystem::exists(cale),"baza temporara exista");std::cout<<"Test integrare retea evaluari: SUCCES\n";return 0;
 }catch(const std::exception& e){db.inchideConexiune();std::filesystem::remove(cale);std::cerr<<e.what()<<'\n';return 1;}
}
