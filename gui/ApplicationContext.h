#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

class ClientEdu;

class ApplicationContext final {
public:
    ApplicationContext(std::string host, std::uint16_t port);
    ~ApplicationContext();

    ApplicationContext(const ApplicationContext&) = delete;
    ApplicationContext& operator=(const ApplicationContext&) = delete;

    void conecteaza();
    void reconecteaza();
    void deconecteaza() noexcept;
    bool verificaPing();

    bool esteConectat() const noexcept;
    ClientEdu& client();

    void salveazaSesiune(int utilizatorId, std::string email, std::string rol);
    void reseteazaSesiune() noexcept;
    bool esteAutentificat() const noexcept;

    const std::string& host() const noexcept;
    std::uint16_t port() const noexcept;
    std::optional<int> utilizatorId() const noexcept;
    const std::string& email() const noexcept;
    const std::string& rol() const noexcept;

private:
    std::string host_;
    std::uint16_t port_{};
    std::shared_ptr<ClientEdu> client_;
    std::optional<int> utilizatorId_;
    std::string email_;
    std::string rol_;
};
