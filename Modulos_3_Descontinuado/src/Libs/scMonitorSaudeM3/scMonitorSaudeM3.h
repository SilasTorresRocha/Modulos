#ifndef SC_MONITOR_SAUDE_M3_H
#define SC_MONITOR_SAUDE_M3_H

#include <Arduino.h>
#include "../../LibsGlobais/scSaudeHardware/scSaudeHardware.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../scGestorSDCardM3/scGestorSDCardM3.h"

class scMonitorSaudeM3 : public scSaudeHardware {
public:
    scMonitorSaudeM3();
    
    // Injeção de dependências do hardware local
    void inicializarLocal(scLogger* logger, scRTCFisicoM3* rtc, scGestorSDCardM3* sdCard);
    
    // Motor de varredura proativa. Executar na Task do Roteamento
    void verificarIntegridadeLocal();
    
    // Indica se a Task da LVGL travou
    void setWatchdogLVGL(bool travado);

    // Getters de contrato 
    bool TemErroCritico();
    String ObterUltimoErro();

private:
    scLogger* _logger;
    scRTCFisicoM3* _rtc;
    scGestorSDCardM3* _sdCard;
    
    // Flags de Saúde Física e Lógica
    bool _rtcMorto;
    bool _sdMorto;
    bool _lvglTravado;
    
    // Travas de Flood de Logs
    bool _alertaEnviadoRTC;
    bool _alertaEnviadoSD;
    bool _alertaEnviadoLVGL;
    
    uint32_t _ultimoCheck;
};

#endif // SC_MONITOR_SAUDE_M3_H
