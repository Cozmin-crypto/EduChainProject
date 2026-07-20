#include "ApplicationContext.h"

#include "ClientEdu.h"
#include "ExceptieEdu.h"

#include <utility>

ApplicationContext::ApplicationContext(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port),
      client_(std::make_shared<ClientEdu>(port_, host_)) {
}

ApplicationContext::~ApplicationContext() {
    deconecteaza();
}

void ApplicationContext::conecteaza() {
    if (!client_->esteConectat()) {
        client_->pornesteNod();
    }
    if (!verificaPing()) {
        client_->opresteNod();
        throw ExceptieEdu("Serverul nu a raspuns corect la PING.");
    }
}

void ApplicationContext::reconecteaza() {
    deconecteaza();
    conecteaza();
}

void ApplicationContext::deconecteaza() noexcept {
    reseteazaSesiune();
    if (!client_ || !client_->esteConectat()) {
        return;
    }
    try {
        client_->deconecteaza();
    } catch (...) {
        client_->opresteNod();
    }
}

bool ApplicationContext::verificaPing() {
    return client_->esteConectat() && client_->trimiteCerere("PING") == "PONG";
}

bool ApplicationContext::esteConectat() const noexcept {
    return client_ && client_->esteConectat();
}

ClientEdu& ApplicationContext::client() {
    if (!client_) {
        throw ExceptieEdu("Clientul nu este disponibil.");
    }
    return *client_;
}

void ApplicationContext::salveazaSesiune(int utilizatorId,
                                         std::string email,
                                         std::string rol) {
    utilizatorId_ = utilizatorId;
    email_ = std::move(email);
    rol_ = std::move(rol);
}

void ApplicationContext::reseteazaSesiune() noexcept {
    utilizatorId_.reset();
    email_.clear();
    rol_.clear();
}

bool ApplicationContext::esteAutentificat() const noexcept {
    return utilizatorId_.has_value();
}

const std::string& ApplicationContext::host() const noexcept { return host_; }
std::uint16_t ApplicationContext::port() const noexcept { return port_; }
std::optional<int> ApplicationContext::utilizatorId() const noexcept { return utilizatorId_; }
const std::string& ApplicationContext::email() const noexcept { return email_; }
const std::string& ApplicationContext::rol() const noexcept { return rol_; }
