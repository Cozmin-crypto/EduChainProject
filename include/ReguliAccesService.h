#pragma once

#include "CursRepository.h"
#include "UtilizatorRepository.h"

class ReguliAccesService {
private:
    UtilizatorRepository& utilizatori;

public:
    explicit ReguliAccesService(UtilizatorRepository& utilizatori);

    UtilizatorInregistrare obtineActor(int actorId);
    void verificaAdministratorSauProprietar(int actorId,
                                            const CursInregistrare& curs);
    void verificaStudent(int actorId);
};
