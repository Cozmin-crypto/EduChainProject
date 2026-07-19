#include "AutentificareService.h"
#include "ClientEdu.h"
#include "ConectorBazaDate.h"
#include "CursRepository.h"
#include "CursService.h"
#include "EvaluareRepository.h"
#include "EvaluareService.h"
#include "ExceptieEdu.h"
#include "InscriereRepository.h"
#include "InscriereService.h"
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
namespace{void v(bool c,const char*m){if(!c)throw std::runtime_error(m);}bool ex(const std::function<void()>&f){try{f();}catch(const ExceptieEdu&){return true;}return false;}void prof(ConectorBazaDate&d,int id){d.executaInterogareParametrizata("INSERT INTO personal(utilizator_id,departament,data_angajarii) VALUES (?,?,?);",{std::to_string(id),"IT","2026-07-19"});d.executaInterogareParametrizata("INSERT INTO profesori(utilizator_id) VALUES (?);",{std::to_string(id)});}void stud(ConectorBazaDate&d,int id){d.executaInterogareParametrizata("INSERT INTO studenti(utilizator_id) VALUES (?);",{std::to_string(id)});}template<class F>void conn(ServerEdu&s,F f){std::exception_ptr e;std::thread t([&]{try{s.proceseazaCerere();}catch(...){e=std::current_exception();}});try{ClientEdu c(s.obtinePort());c.pornesteNod();f(c);if(c.esteConectat())c.deconecteaza();}catch(...){s.opresteNod();t.join();throw;}t.join();if(e)std::rethrow_exception(e);}}
int main(){auto p=std::filesystem::temp_directory_path()/("educhain_retea_inscrieri_"+std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count())+".db");int so=SocketWindows::numarSocketuriActive(),wi=InitializatorWinsock::numarInstanteActive();ConectorBazaDate db;try{db.deschideConexiune(p.string());UtilizatorRepository u(db);CursRepository cr(db);LectieRepository lr(db);EvaluareRepository er(db);InscriereRepository ir(db);InscriereService is(ir,cr,u);CursService cs(cr,u,is);LectieService ls(lr,cr,u,is);EvaluareService es(er,cr,u,is);AutentificareService as(u);int pr=u.adaugaUtilizator("p@x.ro","p","profesor"),st=u.adaugaUtilizator("s@x.ro","s","student");prof(db,pr);stud(db,st);int curs=cs.creeazaCurs({pr,pr,"C",std::nullopt});int lectie=ls.creeazaLectie({pr,curs,"L","text","x",1,1,std::nullopt,std::nullopt});int ev=es.creeazaEvaluare({pr,curs,"E","chestionar",10,true,0,std::nullopt});{ServerEdu server(as,cs,ls,es,is,0);server.pornesteNod();conn(server,[&](ClientEdu&c){CerereEdu q;q.tip=TipCerereEdu::ListeazaCursuriInscrise;v(c.executaCerere(q).cod==CodRezultatEdu::AccesInterzis,"fara login");});conn(server,[&](ClientEdu&c){c.autentifica("s@x.ro","s");v(c.listeazaCursuri().empty(),"student neinscris vede curs");v(ex([&]{c.listeazaLectii(curs);}),"acces lectii neinscris");v(ex([&]{c.listeazaEvaluari(curs);}),"acces evaluari neinscris");c.inscrieLaCurs(curs);v(c.verificaInscriere(st,curs),"verificare inscriere");v(c.listeazaCursuriInscrise().size()==1&&c.listeazaCursuri().size()==1,"listare inscrieri");v(c.obtineLectie(lectie).has_value()&&c.obtineEvaluare(ev).has_value(),"acces continut inscris");v(ex([&]{c.inscrieLaCurs(curs);}),"duplicat");CerereEdu injectie;injectie.tip=TipCerereEdu::InscrieStudentLaCurs;injectie.campuri={{static_cast<std::uint16_t>(CampEdu::CursId),std::to_string(curs)},{static_cast<std::uint16_t>(CampEdu::Rol),"administrator"}};v(c.executaCerere(injectie).cod==CodRezultatEdu::ValidareEsuata,"injectie rol");c.retrageDeLaCurs(curs);v(!c.verificaInscriere(st,curs),"retragere");v(c.trimiteCerere("PING")=="PONG","ping");});conn(server,[&](ClientEdu&c){c.autentifica("p@x.ro","p");c.inscrieStudentLaCurs(st,curs);v(c.listeazaStudentiCurs(curs).size()==1,"profesor listare");});server.opresteNod();}db.inchideConexiune();std::filesystem::remove(p);v(SocketWindows::numarSocketuriActive()==so,"socket");v(InitializatorWinsock::numarInstanteActive()==wi,"winsock");std::cout<<"Test retea inscrieri: SUCCES\n";return 0;}catch(const std::exception&e){db.inchideConexiune();std::filesystem::remove(p);std::cerr<<e.what()<<'\n';return 1;}}
