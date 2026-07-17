#include "ServerEdu.h"

#include "ExceptieEdu.h"

void ServerEdu::pornesteNod() {
    ruleaza = true;
    // TODO: initializarea socket-ului de ascultare va fi implementata ulterior.
}

void ServerEdu::opresteNod() {
    ruleaza = false;
    // TODO: inchiderea socket-ului de ascultare va fi implementata ulterior.
}

void ServerEdu::proceseazaCerere() {
    if (!ruleaza) {
        throw ExceptieEdu("Serverul nu este pornit.");
    }

    // TODO: cererea si rutarea ei catre logica de server nu sunt expuse de API.
}
