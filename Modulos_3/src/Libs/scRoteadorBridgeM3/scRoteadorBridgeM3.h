#ifndef SC_ROTEADOR_BRIDGE_M3_H
#define SC_ROTEADOR_BRIDGE_M3_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "../../LibsGlobais/scTransceptorESPNow/scTransceptorESPNow.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"
#include "../scGestorSDCardM3/scGestorSDCardM3.h"

class scRoteadorBridgeM3 {
public:
    scRoteadorBridgeM3();

    void inicializar(scLogger* logger, 
                     scMQTTLib* mqtt, 
                     scTransceptorESPNow* transceptor, 
                     scGestorRede* rede, 
                     scGestorSDCardM3* sdCard);

    // Motor chamado periodicamente na Task de Rede para esvaziar a fila do SD caso a internet volte
    void atualizar();

    // Invocado quando um pacote chega pelo ESP-NOW (De qualquer modulo: M1, M2...)
    void rotearPacoteDeESPNowParaNuvem(const char* macOrigem, const char* payload);

    // Invocado quando chega um pacote via MQTT da Nuvem (O Backend mandou algo para a casa)
    void rotearPacoteDaNuvemParaESPNow(const String& payloadJSON);

private:
    scLogger* _logger;
    scMQTTLib* _mqtt;
    scTransceptorESPNow* _transceptor;
    scGestorRede* _rede;
    scGestorSDCardM3* _sdCard;
    
    // Auxiliar para extrair o MAC do JSON e converter para uint8_t[]
    bool extrairMACDestino(const String& payload, uint8_t* macDestinoBytes);
};

#endif // SC_ROTEADOR_BRIDGE_M3_H
