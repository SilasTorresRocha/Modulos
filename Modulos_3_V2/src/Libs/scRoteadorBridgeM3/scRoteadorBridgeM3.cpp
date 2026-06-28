#include "scRoteadorBridgeM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include <ArduinoJson.h>
#include "../scGestorSDCardM3/scGestorSDCardM3.h"
#include "../scRadarEcossistemaM3/scRadarEcossistemaM3.h"

scRoteadorBridgeM3::scRoteadorBridgeM3() : 
    _logger(nullptr), _sd(nullptr), _radar(nullptr),
    _head(0), _tail(0) {
    
    // Zera o buffer circular por segurança no boot
    for (int i = 0; i < MAX_FILA_PACOTES; i++) {
        memset(_fila[i].macSender, 0, 6);
        memset(_fila[i].payloadJson, 0, TAM_MAX_PACOTE);
    }
}

void scRoteadorBridgeM3::inicializar(scLogger* logger, scGestorSDCardM3* sd, scRadarEcossistemaM3* radar) {
    _logger = logger;
    _sd = sd;
    _radar = radar;

    if (_logger != nullptr) {
        _logger->info("scRoteadorBridge", "Controlador de Trafego ligado. (Ring Buffer blindado alocando 2.6KB RAM)");
    }
}

bool scRoteadorBridgeM3::_isFilaCheia() {
    return ((_head + 1) % MAX_FILA_PACOTES) == _tail;
}

bool scRoteadorBridgeM3::_isFilaVazia() {
    return _head == _tail;
}

// -----------------------------------------------------------------------------
// [ZONA DE ALTO RISCO - ISR DO HARDWARE RADIO]
// NENHUM byte pode ser alocado na Heap aqui! (Zero Strings). 
// NENHUMA tentativa de SD.open pode rolar aqui (Guru Meditation de colisão SPI).
// -----------------------------------------------------------------------------
void scRoteadorBridgeM3::onPacoteRecebidoEspNow(const uint8_t* macAddr, const uint8_t* data, int len) {
    if (len >= TAM_MAX_PACOTE - 1 || len <= 0) return;

    if (!_isFilaCheia()) {
        // memcpy atua em microssegundos no nivel mais raso da linguagem
        memcpy(_fila[_head].macSender, macAddr, 6);
        
        memcpy(_fila[_head].payloadJson, data, len);
        _fila[_head].payloadJson[len] = '\0'; // Krav magá do C: terminador nulo obrigatorio
        
        // Avanca o anel (head) sem congelar nada
        _head = (_head + 1) % MAX_FILA_PACOTES;
    }
}

// -----------------------------------------------------------------------------
// [ZONA SEGURA - CONTEXTO DO LOOP BARE-METAL]
// -----------------------------------------------------------------------------
void scRoteadorBridgeM3::processar() {
    while (!_isFilaVazia()) {
        // Agora sim: o formating pesado para Char Array (O(n)) no ambiente do loop
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 _fila[_tail].macSender[0], _fila[_tail].macSender[1], _fila[_tail].macSender[2],
                 _fila[_tail].macSender[3], _fila[_tail].macSender[4], _fila[_tail].macSender[5]);

        // Clonagem para limpar o slot do RingBuffer rápido e prosseguir
        char payloadTrabalho[TAM_MAX_PACOTE];
        strncpy(payloadTrabalho, _fila[_tail].payloadJson, TAM_MAX_PACOTE);
        payloadTrabalho[TAM_MAX_PACOTE - 1] = '\0';
        
        _tail = (_tail + 1) % MAX_FILA_PACOTES; // Libera o index circular para o hardware escrever novamente

        // Extrai o 'tipo' do JSON para uso interno no Radar (M1, M2, M4)
        StaticJsonDocument<512> doc;
        DeserializationError erro = deserializeJson(doc, payloadTrabalho);
        const char* tipoDetectado = "UKN";
        if (!erro) {
            tipoDetectado = doc["tipo"] | "UKN";
        }

        // ==========================================
        // Roteamento Oficial
        // ==========================================
        
        // 1. Apita no Dead Man's Switch (Radar) para dizer que esse nó tá vivo!
        if (_radar != nullptr) {
            _radar->registrarPing(macStr, true, tipoDetectado); // true = foi pego pelo ESP-NOW
        }

        // 2. Salva o pacote na Fila Idempotente offline (SD Card) 
        // OBS: Num cenário WiFi saudável, o loop principal também vai drenar o SD Card para Nuvem.
        if (_sd != nullptr) {
            _sd->enfileirarOffline(payloadTrabalho);
        }

        if (_logger != nullptr) {
            char logBuf[100];
            snprintf(logBuf, sizeof(logBuf), "Pacote rotacionado da ISR -> Roteador (Origem: %s)", macStr);
            _logger->info("scRoteadorBridge", logBuf);
        }
    }
}
