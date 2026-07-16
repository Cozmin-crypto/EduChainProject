#pragma once

#include <string>
#include <exception>

class ExceptieEdu : public std::exception {
private:
    std::string mesajEroare;

public:
    explicit ExceptieEdu(std::string mesaj);
    const char* what() const noexcept override;
};
