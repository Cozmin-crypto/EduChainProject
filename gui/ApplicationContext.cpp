#include "ApplicationContext.h"

#include "ClientEdu.h"
#include "ExceptieEdu.h"
#include "ProtocolEdu.h"

#include <iostream>
#include <utility>

ApplicationContext::ApplicationContext(std::string host, std::uint16_t port)
    : host_(std::move(host)), port_(port),
      client_(std::make_shared<ClientEdu>(port_, host_)) {
}

ApplicationContext::~ApplicationContext() {
    deconecteaza();
}

void ApplicationContext::conecteaza() {
    try {
        if (!client_->esteConectat()) {
            client_->pornesteNod();
        }
        std::clog << "[CLIENT] Protocol version: " << versiuneProtocolEdu << '\n'
                  << "[CLIENT] Handshake: sending PING\n";
        if (!verificaPing()) {
            client_->opresteNod();
            throw ExceptieEdu("Serverul nu a raspuns corect la PING.");
        }
        ultimaEroareConexiune_.clear();
        std::clog << "[CLIENT] Handshake: success\n"
                  << "[CLIENT] Final connected state: true\n";
    } catch (const std::exception& exceptie) {
        ultimaEroareConexiune_ = exceptie.what();
        std::clog << "[CLIENT] Handshake: failure: " << ultimaEroareConexiune_ << '\n'
                  << "[CLIENT] Final connected state: false\n";
        throw;
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
                                         std::string rol,
                                         std::string nume,
                                         std::string prenume) {
    utilizatorId_ = utilizatorId;
    email_ = std::move(email);
    rol_ = std::move(rol);
    nume_ = std::move(nume);
    prenume_ = std::move(prenume);
}

void ApplicationContext::reseteazaSesiune() noexcept {
    utilizatorId_.reset();
    email_.clear();
    rol_.clear();
    nume_.clear();
    prenume_.clear();
}

bool ApplicationContext::esteAutentificat() const noexcept {
    return utilizatorId_.has_value();
}

const std::string& ApplicationContext::host() const noexcept { return host_; }
std::uint16_t ApplicationContext::port() const noexcept { return port_; }
std::optional<int> ApplicationContext::utilizatorId() const noexcept { return utilizatorId_; }
const std::string& ApplicationContext::email() const noexcept { return email_; }
const std::string& ApplicationContext::rol() const noexcept { return rol_; }
const std::string& ApplicationContext::nume() const noexcept { return nume_; }
const std::string& ApplicationContext::prenume() const noexcept { return prenume_; }
const std::string& ApplicationContext::ultimaEroareConexiune() const noexcept { return ultimaEroareConexiune_; }
std::string ApplicationContext::numeComplet() const {
    return prenume_.empty() ? nume_ : (nume_.empty() ? prenume_ : prenume_ + " " + nume_);
}
