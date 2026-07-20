#include "InscriereService.h"
#include "CursRepository.h"
#include "ExceptieEdu.h"
InscriereService::InscriereService(InscriereRepository&i,CursRepository&c,UtilizatorRepository&u):inscrieri(i),cursuri(c),reguli(u){}
void InscriereService::verificaAdministrare(int actorId,int studentId,int cursId){const auto actor=reguli.obtineActor(actorId);if(actor.rol=="student"){if(actor.id!=studentId)throw ExceptieEdu("Studentul nu poate administra inscrierea altui student.");return;}const auto c=cursuri.cautaDupaId(cursId);if(!c)throw ExceptieEdu("Cursul specificat nu exista.");reguli.verificaAdministratorSauProprietar(actorId,*c);}
void InscriereService::inscrieStudent(int a,int s,int c){verificaAdministrare(a,s,c);inscrieri.inscrieStudentLaCurs(s,c);}
void InscriereService::retrageStudent(int a,int s,int c){verificaAdministrare(a,s,c);inscrieri.retrageStudentDeLaCurs(s,c);}
std::vector<CursInregistrare> InscriereService::listeazaCursuriInscrise(int a){reguli.verificaStudent(a);return inscrieri.listeazaCursuriStudent(a);}
std::vector<UtilizatorInregistrare> InscriereService::listeazaStudentiCurs(int a,int c){const auto curs=cursuri.cautaDupaId(c);if(!curs)throw ExceptieEdu("Cursul specificat nu exista.");reguli.verificaAdministratorSauProprietar(a,*curs);return inscrieri.listeazaStudentiCurs(c);}
bool InscriereService::verificaInscriere(int a,int s,int c){const auto actor=reguli.obtineActor(a);if(actor.rol=="student"&&actor.id!=s)throw ExceptieEdu("Studentul nu poate vedea inscrierea altui student.");if(actor.rol!="student"){const auto curs=cursuri.cautaDupaId(c);if(!curs)throw ExceptieEdu("Cursul specificat nu exista.");reguli.verificaAdministratorSauProprietar(a,*curs);}return inscrieri.esteInscris(s,c);}
void InscriereService::verificaAccesStudentLaCurs(int a,int c){const auto actor=reguli.obtineActor(a);if(actor.rol=="student"&&!inscrieri.esteInscris(a,c))throw ExceptieEdu("Studentul nu este inscris la curs.");}
