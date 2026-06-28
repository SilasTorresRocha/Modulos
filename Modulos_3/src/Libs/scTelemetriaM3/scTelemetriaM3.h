#ifndef SC_TELEMETRIA_M3_H
#define SC_TELEMETRIA_M3_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../scMonitorSaudeM3/scMonitorSaudeM3.h"
#include "../scRoteadorBridgeM3/scRoteadorBridgeM3.h"

class scTelemetriaM3 {
public:
    scTelemetriaM3();

    void inicializar(scLogger* logger, 
                     scGestorRede* rede, 
                     scRelogioSincronizado* relogio,
                     scRTCFisicoM3* rtc,
                     scMonitorSaudeM3* monitorSaude,
                     scRoteadorBridgeM3* roteador,
                     const String& macPlaca);

    // Constrói e injeta o JSON no roteador (que decide se vai pra nuvem ou pro SD)
    void despacharTelemetriaM3();

private:
    scLogger* _logger;
    scGestorRede* _rede;
    scRelogioSincronizado* _relogio;
    scRTCFisicoM3* _rtc;
    scMonitorSaudeM3* _monitorSaude;
    scRoteadorBridgeM3* _roteador;
    String _macPlaca;
};

#endif // SC_TELEMETRIA_M3_H
