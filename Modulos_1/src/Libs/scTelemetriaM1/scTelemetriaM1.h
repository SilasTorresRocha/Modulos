#ifndef SC_TELEMETRIA_M1_H
#define SC_TELEMETRIA_M1_H

#include <Arduino.h>
#include "../../LibsGlobais/scTelemetriaBase/scTelemetriaBase.h"


class scMonitorGasM1;
class scLeitorTermicoM1;
class scGestorAlarmesM1;
class scMonitorSaudeM1;
class scGestorRede;
class scTransceptorESPNow;
class scLogger;
class scMQTTLib;

class scTelemetriaM1 : public scTelemetriaBase {
public:
    scTelemetriaM1();
    
    // Injeção de todas as camadas vitais do sensor
    void inicializar(scMQTTLib* mqtt, scLogger* logger, scMonitorSaudeM1* saude, 
                     scGestorRede* gestorRede, scTransceptorESPNow* transceptor, 
                     scMonitorGasM1* gas, scLeitorTermicoM1* termico, scGestorAlarmesM1* alarmes,
                     const uint8_t* macHub);
                     
    // Varre as classes, minifica no JSON contratual e despacha
    void despacharTelemetriaM1();

private:
    scMonitorGasM1* _gas;
    scLeitorTermicoM1* _termico;
    scGestorAlarmesM1* _alarmes;
    scMonitorSaudeM1* _saudeM1;
    scGestorRede* _gestorRede;
};

#endif // SC_TELEMETRIA_M1_H
