#include "scTransceptorESPNow.h"
#include "../scLogger/scLogger.h"

scTransceptorESPNow* scTransceptorESPNow::_instanciaGlobal = nullptr;

scTransceptorESPNow::scTransceptorESPNow() {
    _logger = nullptr;
    _acaoRecebimento = nullptr;
    _usarCriptografia = false;
    _instanciaGlobal = this;
}

// Helper para preencher o array de bytes do MAC formato "AA:BB:CC:DD:EE:11"
void scTransceptorESPNow::macParaBuffer(const uint8_t* macBytes, char* buffer) {
    snprintf(buffer, 18, "%02X:%02X:%02X:%02X:%02X:%02X", 
             macBytes[0], macBytes[1], macBytes[2], 
             macBytes[3], macBytes[4], macBytes[5]);
}

void scTransceptorESPNow::inicializar(scLogger* logger) {
    _logger = logger;

    if (esp_now_init() != 0) {
        if (_logger) _logger->erro("ESPNOW", "Falha critica ao inicializar o hardware de radio.");
        return;
    }

    if (_logger) _logger->info("ESPNOW", "Hardware inicializado com sucesso.");

#if defined(ESP8266)
    esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
    esp_now_register_recv_cb(scTransceptorESPNow::callbackRecebimento);
    esp_now_register_send_cb(scTransceptorESPNow::callbackEnvio);
#elif defined(ESP32)
    esp_now_register_recv_cb(scTransceptorESPNow::callbackRecebimento);
    esp_now_register_send_cb(scTransceptorESPNow::callbackEnvio);
#endif
}

void scTransceptorESPNow::configurarCriptografia(String pmk, String lmk) {
    if (pmk.length() >= 16 && lmk.length() >= 16) {
        memcpy(_pmk, pmk.c_str(), 16);
        memcpy(_lmk, lmk.c_str(), 16);
        _usarCriptografia = true;

#if defined(ESP8266)
        // No ESP8266, a PMK é definida por esta função específica (Key-of-Keys)
        esp_now_set_kok(_pmk, 16);
        if (_logger) _logger->info("ESPNOW", "Criptografia de Hardware (ESP8266) ativada.");
#elif defined(ESP32)
        esp_now_set_pmk(_pmk);
        if (_logger) _logger->info("ESPNOW", "Criptografia de Hardware (ESP32) ativada.");
#endif

    } else {
        if (_logger) _logger->erro("ESPNOW", "As chaves fornecidas nao possuem 16 bytes. Criptografia abortada!");
        _usarCriptografia = false;
    }
}

void scTransceptorESPNow::adicionarPeerDinamico(uint8_t* mac) {
#if defined(ESP8266)
    if (!esp_now_is_peer_exist(mac)) {
        uint8_t canalAtual = WiFi.channel();
        int r = 0;
        if (_usarCriptografia) {
            r = esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, canalAtual, _lmk, 16);
        } else {
            r = esp_now_add_peer(mac, ESP_NOW_ROLE_COMBO, canalAtual, NULL, 0);
        }
        
        if (r == 0) {
            if (_logger) _logger->info("ESPNOW", "Novo P2P (ESP8266) cadastrado.");
        } else {
            if (_logger) _logger->erro("ESPNOW", "Falha ao alocar Peer ESP8266. Retorno: " + String(r));
        }
    }
#elif defined(ESP32)
    if (!esp_now_is_peer_exist(mac)) {
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, mac, 6);
        peerInfo.channel = WiFi.channel(); // Acompanha a iteracao da scGestorRede
        
        if (_usarCriptografia) {
            peerInfo.encrypt = true;
            memcpy(peerInfo.lmk, _lmk, 16);
        } else {
            peerInfo.encrypt = false;
        }

        if (esp_now_add_peer(&peerInfo) == ESP_OK) {
            if (_logger) _logger->info("ESPNOW", "Novo P2P (ESP32) cadastrado.");
        }
    }
#endif
}

bool scTransceptorESPNow::enviarPacote(uint8_t* macDestino, const String& payload) {
    if (payload.length() > 250) {
        if (_logger) _logger->erro("ESPNOW", "Pacote Abortado! Payload excede limite de 250 bytes.");
        return false;
    }

    adicionarPeerDinamico(macDestino);

    uint8_t* payloadBytes = (uint8_t*)payload.c_str();
    int size = payload.length();

#if defined(ESP8266)
    return (esp_now_send(macDestino, payloadBytes, size) == 0);
#elif defined(ESP32)
    return (esp_now_send(macDestino, payloadBytes, size) == ESP_OK);
#endif
}

void scTransceptorESPNow::definirRecebimento(AcaoRecebimento acao) {
    _acaoRecebimento = acao;
}

// =========================================================================
// CALLBACKS (RODAM EM MODO INTERRUPÇÃO - MÁXIMA PERFORMANCE EXIGIDA)
// =========================================================================

#if defined(ESP8266)
void scTransceptorESPNow::callbackRecebimento(uint8_t * mac, uint8_t *dadosRecebidos, uint8_t tamanho) {
    if (_instanciaGlobal && _instanciaGlobal->_acaoRecebimento) {
        char payload[251];
        memcpy(payload, dadosRecebidos, tamanho);
        payload[tamanho] = '\0';
        
        char macOrigem[18];
        _instanciaGlobal->macParaBuffer(mac, macOrigem);
        
        _instanciaGlobal->_acaoRecebimento(macOrigem, payload);
    }
}
void scTransceptorESPNow::callbackEnvio(uint8_t *macDestino, uint8_t statusEnvio) {}

#elif defined(ESP32)
void scTransceptorESPNow::callbackRecebimento(const uint8_t *mac_addr, const uint8_t *dadosRecebidos, int tamanho) {
    if (_instanciaGlobal && _instanciaGlobal->_acaoRecebimento) {
        char payload[251];
        memcpy(payload, dadosRecebidos, tamanho);
        payload[tamanho] = '\0';
        
        char macOrigem[18];
        _instanciaGlobal->macParaBuffer(mac_addr, macOrigem);
        
        _instanciaGlobal->_acaoRecebimento(macOrigem, payload);
    }
}
void scTransceptorESPNow::callbackEnvio(const uint8_t *macDestino, esp_now_send_status_t statusEnvio) {}
#endif
