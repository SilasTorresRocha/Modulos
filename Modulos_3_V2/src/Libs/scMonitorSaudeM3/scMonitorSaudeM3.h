#ifndef SC_MONITOR_SAUDE_M3_H
#define SC_MONITOR_SAUDE_M3_H

#include <Arduino.h>
#include "../../LibsGlobais/scSaudeHardware/scSaudeHardware.h" // Lib Global (Watchdog e Memoria)

class scLogger;
class scRTCFisicoM3;
class scGestorSDCardM3;
class scGestorBuzzerM3;

class scMonitorSaudeM3 {
private:
    scSaudeHardware _saudeGlobal;
    scLogger* _logger;
    scRTCFisicoM3* _rtc;
    scGestorSDCardM3* _sd;
    scGestorBuzzerM3* _buzzer;

    uint32_t _inicioLoopGrafico;
    bool _alertaMemoriaDisparado;
    bool _alertaI2CDisparado;
    bool _alertaSPIDisparado;

public:
    scMonitorSaudeM3();

    // Injeta todas as dependencias criticas de hardware para inspecao
    void inicializar(scLogger* logger, scRTCFisicoM3* rtc, scGestorSDCardM3* sd, scGestorBuzzerM3* buzzer);

    // Deve ser chamado a cada iteracao do Super Loop para bater o Watchdog
    void processar();

    // ==========================================
    // Monitoramento do Motor Grafico (Anti-Engasgo)
    // ==========================================
    void iniciarMedicaoLoopGrafico();
    void finalizarMedicaoLoopGrafico();

    // Devolve se o M3 esta beirando o colapso
    bool isCritico();
    
    // Devolve String estatica (Zero Heap) para ser injetada no JSON de telemetria
    void preencherTelemetria(char* bufferSaida, size_t maxLen);
};

#endif // SC_MONITOR_SAUDE_M3_H
