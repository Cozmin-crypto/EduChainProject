#include "InscriereService.h"
#include "CursRepository.h"
#include "ExceptieEdu.h"
InscriereService::InscriereService(InscriereRepository&i,CursRepository&c,UtilizatorRepository&u):inscrieri(i),cursuri(c),utilizatori(u),reguli(u){}
void InscriereService::verificaAdministrare(int actorId,int studentId,int cursId){
 if(studentId<=0)throw ExceptieEdu("Id-ul studentului este invalid.");
 if(cursId<=0)throw ExceptieEdu("Id-ul cursului este invalid.");
 const auto actor=reguli.obtineActor(actorId);
 const auto curs=cursuri.cautaDupaId(cursId);
 if(!curs)throw ExceptieEdu("Cursul specificat nu exista.");
 const auto student=utilizatori.cautaDupaId(studentId);
 if(!student)throw ExceptieEdu("Studentul nu exista.");
 if(actor.rol=="profesor"&&actor.id==studentId)throw ExceptieEdu("Profesorul nu poate fi inscris ca student.");
 if(student->rol!="student")throw ExceptieEdu("Utilizatorul selectat nu este student.");
 if(actor.rol=="student"){
  if(actor.id!=studentId)throw ExceptieEdu("Studentul nu poate administra inscrierea altui student.");
  return;
 }
 reguli.verificaProfesorProprietar(actorId,*curs);
}
void InscriereService::inscrieStudent(int a,int s,int c){verificaAdministrare(a,s,c);if(inscrieri.esteInscris(s,c))throw ExceptieEdu("Studentul este deja inscris la acest curs.");inscrieri.inscrieStudentLaCurs(s,c);}
void InscriereService::retrageStudent(int a,int s,int c){verificaAdministrare(a,s,c);inscrieri.retrageStudentDeLaCurs(s,c);}
std::vector<CursInregistrare> InscriereService::listeazaCursuriInscrise(int a){reguli.verificaStudent(a);return inscrieri.listeazaCursuriStudent(a);}
std::vector<CursInregistrare> InscriereService::listeazaCursuriDisponibile(int a){reguli.verificaStudent(a);return inscrieri.listeazaCursuriDisponibile(a);}
std::vector<UtilizatorInregistrare> InscriereService::listeazaStudentiCurs(int a,int c){const auto curs=cursuri.cautaDupaId(c);if(!curs)throw ExceptieEdu("Cursul specificat nu exista.");reguli.verificaProfesorProprietar(a,*curs);return inscrieri.listeazaStudentiCurs(c);}
bool InscriereService::verificaInscriere(int a,int s,int c){verificaAdministrare(a,s,c);return inscrieri.esteInscris(s,c);}
void InscriereService::verificaAccesStudentLaCurs(int a,int c){const auto actor=reguli.obtineActor(a);if(actor.rol=="student"&&!inscrieri.esteInscris(a,c))throw ExceptieEdu("Studentul nu este inscris la curs.");}
