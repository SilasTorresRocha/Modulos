#ifndef SC_TELEMETRIA_M2_H
#define SC_TELEMETRIA_M2_H

#include <Arduino.h>
#include "../../LibsGlobais/scTelemetriaBase/scTelemetriaBase.h"

// Forward Declarations para preservar tempo de compilação
class scGestorReles;
class scGestorRede;
class scMonitorSaudeM2;
class scTransceptorESPNow;
class scLogger;
class scMQTTLib;

class scTelemetriaM2 : public scTelemetriaBase {
public:
    scTelemetriaM2();
    
    // Injeta a base global de rede e os gerenciadores fisicos locais do M2
    void inicializar(scMQTTLib* mqtt, scLogger* logger, scMonitorSaudeM2* saude, 
                     scGestorRede* gestorRede, scTransceptorESPNow* transceptor, 
                     scGestorReles* reles, const uint8_t* macHub);
                     
    // Métodos para abastecer o Motor Grafico (scWatchfacesM2)
    String obterIP();
    String obterMAC();
    String obterStatusRede();
    int obterRSSI();
    
    // Ponte local para exibir a temp do M1 na tela do M2
    void setTempM1(float t);
    float obterTempM1();

    // Rotina principal que minifica e repassa os dados via base global
    void despacharTelemetriaM2();

private:
    scGestorReles* _reles;
    scGestorRede* _gestorRede;
    scMonitorSaudeM2* _saudeM2;
    
    float _tempM1Cache;
};

#endif // SC_TELEMETRIA_M2_H
