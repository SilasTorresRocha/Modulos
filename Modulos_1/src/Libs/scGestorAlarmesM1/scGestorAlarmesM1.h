#ifndef SC_GESTOR_ALARMES_M1_H
#define SC_GESTOR_ALARMES_M1_H

#include <Arduino.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scAvisosSonoros/scAvisosSonoros.h"

class scGestorAlarmesM1 {
public:
    scGestorAlarmesM1();

    // Injeção mandatória do log e da engine global de sons
    void inicializar(scLogger* logger, scAvisosSonoros* avisosSonoros);

    // Recebe e atualiza os limites de tempo (contrato MQTT)
    void setAlarmes(uint32_t preparoSegundos, uint32_t criticoSegundos);

    // Listeners do estado das outras bibliotecas.
    // Usados pelas Callbacks do Forno e do Gás lá na main.
    void setEstadoForno(bool ligado);
    void setEstadoVazamento(bool vazando);

    // Motor inteligente de verificação de limites. Deve ir no loop()
    void processar();

    // Getters vitais requeridos pela Telemetria
    uint32_t TempoForno() const;
    uint32_t AlmPrep() const;
    uint32_t AlmCrit() const;

private:
    scLogger* _logger;
    scAvisosSonoros* _avisos;

    uint32_t _almPrepSegundos;
    uint32_t _almCritSegundos;

    bool _fornoLigado;
    bool _vazamentoGas;

    uint32_t _inicioFornoMillis;
    PadraoSom _alarmeAtual;
};

#endif // SC_GESTOR_ALARMES_M1_H
