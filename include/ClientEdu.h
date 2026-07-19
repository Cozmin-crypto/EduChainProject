#pragma once

#include "ManagerSocket.h"
#include "ProtocolEdu.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

struct CerereActualizareLectieClient {
    int lectieId{};
    int cursId{};
    std::string nume;
    TipLectieEdu tip{TipLectieEdu::Text};
    std::string continut;
    long long dimensiuneOcteti{};
    std::optional<long long> numarCuvinte;
    std::optional<long long> durata;
    std::optional<std::string> codec;
};

struct CerereSalvareEvaluareClient {
    int cursId{};
    std::string nume;
    long long limitaTimp{};
    bool esteObligatorie{};
    std::optional<long long> numarIntrebari;
    std::optional<double> pondere;
};

struct CerereActualizareEvaluareClient : CerereSalvareEvaluareClient {
    int evaluareId{};
    TipEvaluareEdu tip{TipEvaluareEdu::Chestionar};
};

class ClientEdu : public ManagerSocket {
private:
    bool conectat{};
    bool autentificat{};
    std::string rolAutentificat;
    std::string ultimulRaspuns;
    std::uint32_t urmatorulIdCerere{1};

    std::uint32_t genereazaIdCerere();
    static void verificaSucces(const RaspunsEdu& raspuns);

public:
    explicit ClientEdu(std::uint16_t port = 0,
                       std::string adresaIp = "127.0.0.1");
    ~ClientEdu() noexcept override;

    void pornesteNod() override;
    void opresteNod() override;
    void trimiteCerere();
    std::string trimiteCerere(const std::string& mesaj);
    RaspunsEdu executaCerere(CerereEdu cerere);

    RaspunsEdu autentifica(const std::string& email, const std::string& parola);
    int inregistreaza(const std::string& nume, const std::string& prenume,
                      const std::string& email, const std::string& parola,
                      const std::string& rol);
    int inregistreazaStudent(const std::string&,const std::string&,const std::string&,const std::string&);
    std::vector<CursPublicEdu> listeazaCursuri();
    std::optional<CursPublicEdu> obtineCurs(int cursId);
    int creeazaCurs(const std::string& nume,
                    std::optional<int> parinteId = std::nullopt,
                    std::optional<int> proprietarId = std::nullopt);
    void actualizeazaCurs(int cursId,
                          const std::string& nume,
                          std::optional<int> parinteId = std::nullopt);
    void stergeCurs(int cursId);
    std::vector<LectiePublicEdu> listeazaLectii(int cursId);
    std::optional<LectiePublicEdu> obtineLectie(int lectieId);
    int creeazaLectieText(int cursId,
                          const std::string& nume,
                          const std::string& continut,
                          long long dimensiuneOcteti,
                          long long numarCuvinte);
    int creeazaLectieVideo(int cursId,
                           const std::string& nume,
                           const std::string& continut,
                           long long dimensiuneOcteti,
                           long long durata,
                           const std::string& codec);
    void actualizeazaLectie(const CerereActualizareLectieClient& cerere);
    void stergeLectie(int lectieId);
    std::vector<EvaluarePublicEdu> listeazaEvaluari(int cursId);
    std::optional<EvaluarePublicEdu> obtineEvaluare(int evaluareId);
    int creeazaChestionar(const CerereSalvareEvaluareClient& cerere);
    int creeazaExamenFinal(const CerereSalvareEvaluareClient& cerere);
    void actualizeazaEvaluare(const CerereActualizareEvaluareClient& cerere);
    void stergeEvaluare(int evaluareId);
    std::vector<IntrebarePublicEdu> listeazaIntrebari(int evaluareId);
    int adaugaIntrebare(int evaluareId, const std::string& enunt,
                        const std::string& raspunsCorect,
                        double punctajMaxim, long long ordine);
    void actualizeazaIntrebare(int evaluareId, int intrebareId,
                               const std::string& enunt,
                               const std::string& raspunsCorect,
                               double punctajMaxim,
                               long long ordine);
    void stergeIntrebare(int evaluareId, int intrebareId);
    int pornesteIncercare(int evaluareId);
    std::optional<IncercarePublicEdu> obtineIncercare(int incercareId);
    void salveazaRaspuns(int incercareId, int intrebareId,
                         const std::string& continut);
    IncercarePublicEdu finalizeazaIncercare(int incercareId);
    void inscrieLaCurs(int cursId);
    void retrageDeLaCurs(int cursId);
    void inscrieStudentLaCurs(int studentId,int cursId);
    void retrageStudentDeLaCurs(int studentId,int cursId);
    std::vector<CursPublicEdu> listeazaCursuriInscrise();
    std::vector<StudentPublicEdu> listeazaStudentiCurs(int cursId);
    bool verificaInscriere(int studentId,int cursId);
    void deconecteaza();

    bool esteConectat() const noexcept;
    bool esteAutentificat() const noexcept;
    const std::string& obtineRolAutentificat() const noexcept;
    const std::string& obtineUltimulRaspuns() const noexcept;
};
