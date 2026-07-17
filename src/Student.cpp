#include "Student.h"

void Student::autentificare() {
    // Autentificarea necesita validarea credentialelor prin server.
}

std::string Student::obtineRol() const {
    return "Student";
}

void Student::trimiteEvaluare(int idEvaluare) {
    (void)idEvaluare;
    // Trimiterea evaluarii necesita comunicarea cu serverul.
}

bool Student::operator==(const Student& altul) const {
    return UtilizatorBaza::operator==(altul);
}
