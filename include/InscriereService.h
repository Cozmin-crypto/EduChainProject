#pragma once
#include "InscriereRepository.h"
#include "ReguliAccesService.h"
class CursRepository;
class UtilizatorRepository;
class InscriereService {
    InscriereRepository& inscrieri; CursRepository& cursuri; ReguliAccesService reguli;
    void verificaAdministrare(int actorId,int studentId,int cursId);
public:
    InscriereService(InscriereRepository&,CursRepository&,UtilizatorRepository&);
    void inscrieStudent(int actorId,int studentId,int cursId);
    void retrageStudent(int actorId,int studentId,int cursId);
    std::vector<CursInregistrare> listeazaCursuriInscrise(int actorId);
    std::vector<UtilizatorInregistrare> listeazaStudentiCurs(int actorId,int cursId);
    bool verificaInscriere(int actorId,int studentId,int cursId);
    void verificaAccesStudentLaCurs(int actorId,int cursId);
};
