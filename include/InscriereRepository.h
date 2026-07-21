#pragma once
#include "CursRepository.h"
#include "UtilizatorRepository.h"
#include <string>
#include <vector>
class ConectorBazaDate;
struct InscriereInregistrare { int cursId{}; int studentId{}; std::string inscrisLa; };
class InscriereRepository {
    ConectorBazaDate& conector;
    void valideazaStudent(int studentId);
    void valideazaCurs(int cursId);
public:
    explicit InscriereRepository(ConectorBazaDate& conector);
    void inscrieStudentLaCurs(int studentId,int cursId);
    void retrageStudentDeLaCurs(int studentId,int cursId);
    bool esteInscris(int studentId,int cursId);
    std::vector<CursInregistrare> listeazaCursuriStudent(int studentId);
    std::vector<CursInregistrare> listeazaCursuriDisponibile(int studentId);
    std::vector<UtilizatorInregistrare> listeazaStudentiCurs(int cursId);
};
