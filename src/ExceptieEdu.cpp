#include "ExceptieEdu.h"

#include <utility>

ExceptieEdu::ExceptieEdu(std::string mesaj)
    : mesajEroare(std::move(mesaj)) {
}

const char* ExceptieEdu::what() const noexcept {
    return mesajEroare.c_str();
}
