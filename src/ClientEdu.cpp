#include "ClientEdu.h"

#include "ExceptieEdu.h"

void ClientEdu::pornesteNod() {
    stareConexiune = "conectat";
    // TODO: deschiderea socket-ului va fi implementata in modulul de retea.
}

void ClientEdu::opresteNod() {
    stareConexiune = "deconectat";
    // TODO: inchiderea socket-ului va fi implementata in modulul de retea.
}

void ClientEdu::trimiteCerere() {
    if (stareConexiune != "conectat") {
        throw ExceptieEdu("Clientul nu este conectat la server.");
    }

    // TODO: cererea nu este expusa de semnatura si va fi transmisa prin socket.
}
