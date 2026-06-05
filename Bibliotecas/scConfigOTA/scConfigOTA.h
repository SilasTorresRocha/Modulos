#ifndef SC_CONFIG_OTA_H
#define SC_CONFIG_OTA_H

#include <Arduino.h>

class scArmazenamentoLocal;
class scLogger; 
class scMQTTLib;

class scConfigOTA {
public:
    scConfigOTA();
    
    // Inicia dependências, callbacks do OTA Local e define o nome da placa
    void inicializar(scArmazenamentoLocal* armazenamento, scLogger* logger, scMQTTLib* mqtt, String nomeModulo);
    
    // Deve ser chamada iterativamente no loop() para receber sketchs da IDE local
    void tratarLocalOTA();
    
    // Executada apenas uma vez no setup() (ou após o Wi-Fi conectar).
    // Evita Boot Loop: Só baixa se a URL for nova.
    void verificarAtualizacaoBoot();
    
    // Chamada pelo scDespachanteComandos quando o Backend MQTT empurra um novo binário
    void agendarNovaURL(String novaUrl);

private:
    scArmazenamentoLocal* _armazenamento;
    scLogger* _logger;
    scMQTTLib* _mqtt;
    String _nomeModulo;
};

#endif // SC_CONFIG_OTA_H
