#ifndef SC_ROTEADOR_BRIDGE_M3_H
#define SC_ROTEADOR_BRIDGE_M3_H

#include <Arduino.h>
#include <string.h>

class scLogger;
class scGestorSDCardM3;
class scRadarEcossistemaM3;

#define MAX_FILA_PACOTES 10
#define TAM_MAX_PACOTE 256

// Estrutura atômica para transitar do contexto de ISR para o Loop
struct PacoteISR {
    uint8_t macSender[6];
    char payloadJson[TAM_MAX_PACOTE];
};

class scRoteadorBridgeM3 {
private:
    scLogger* _logger;
    scGestorSDCardM3* _sd;
    scRadarEcossistemaM3* _radar;

    // Ring Buffer (Cesto) cravado na memoria RAM para evitar Heap Fragmentation
    PacoteISR _fila[MAX_FILA_PACOTES];
    volatile uint8_t _head; // Aponta onde a Interrupção vai escrever
    volatile uint8_t _tail; // Aponta de onde o Super Loop vai ler

    bool _isFilaCheia();
    bool _isFilaVazia();

public:
    scRoteadorBridgeM3();

    void inicializar(scLogger* logger, scGestorSDCardM3* sd, scRadarEcossistemaM3* radar);

    // ==========================================
    // ALERTA DE ISR (Interrupt Service Routine)
    // ==========================================
    // Este metodo é invocado nativamente pelo silício de rádio (onDataRecv do ESP-NOW).
    // Jamais execute IO de Disco, Print ou conversão de Strings pesada aqui!
    void onPacoteRecebidoEspNow(const uint8_t* macAddr, const uint8_t* data, int len);

    // O Motor que escoa a fila rotacionando ponteiros e processando no Super Loop.
    void processar();

    // Envia um pacote para todos os módulos (Broadcast) via rádio ESP-NOW
    void enviarBroadcast(const char* payloadJson);
};

#endif // SC_ROTEADOR_BRIDGE_M3_H
