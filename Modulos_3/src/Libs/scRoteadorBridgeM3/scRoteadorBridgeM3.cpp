#include "scRoteadorBridgeM3.h"
#include <WiFi.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif

scRoteadorBridgeM3::scRoteadorBridgeM3() {
    _logger = nullptr;
    _mqtt = nullptr;
    _transceptor = nullptr;
    _rede = nullptr;
    _sdCard = nullptr;
}

void scRoteadorBridgeM3::inicializar(scLogger* logger, scMQTTLib* mqtt, scTransceptorESPNow* transceptor, scGestorRede* rede, scGestorSDCardM3* sdCard) {
    _logger = logger;
    _mqtt = mqtt;
    _transceptor = transceptor;
    _rede = rede;
    _sdCard = sdCard;

    if (!_logger) {
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); yield(); }
    }
}

void scRoteadorBridgeM3::atualizar() {
    // Se não tivermos internet, aborta o flush
    if (_rede->estaEmFallback()) return;
    if (!_mqtt->internetDisponivel()) return;

    // Se a rede esta estavel, tentamos ler 1 linha do banco offline por vez (sem bloquear a Task)
    if (_sdCard && !_sdCard->FalhaComunicacao()) {
        String linhaPend = _sdCard->extrairProximaLinhaOffline();
        
        if (linhaPend.length() > 5) { // JSON minimo valido
            // Dispara para a nuvem
            _logger->info("BRIDGE", "Descarregando telemetria atrasada do SD: " + linhaPend);
            _mqtt->enviarJSON(linhaPend);
            
            // Confirma o avanco do cursor
            _sdCard->confirmarEnvioLinha();
        }
    }
}

void scRoteadorBridgeM3::rotearPacoteDeESPNowParaNuvem(const char* macOrigem, const char* payload) {
    _logger->info("BRIDGE", "Pacote Recebido via ESP-NOW de [" + String(macOrigem) + "]");

    // 1. Se estivermos sem internet, o Hub guarda o pacote no banco de dados para enviar depois
    if (_rede->estaEmFallback() || !_mqtt->internetDisponivel()) {
        if (_sdCard) {
            _logger->warn("BRIDGE", "Internet offline. Guardando pacote de [" + String(macOrigem) + "] no SD Card.");
            _sdCard->gravarTelemetriaOffline(String(payload));
        } else {
            _logger->erro("BRIDGE", "Pacote PERDIDO! Sem internet e sem SD Card montado.");
        }
    } 
    // 2. Se a internet estiver viva, despacha imediatamente (Bypass)
    else {
        _logger->info("BRIDGE", "Roteando diretamente para MQTT (Nuvem).");
        _mqtt->enviarJSON(String(payload));
    }
}

void scRoteadorBridgeM3::rotearPacoteDaNuvemParaESPNow(const String& payloadJSON) {
    _logger->info("BRIDGE", "Ordem recebida da Nuvem.");

    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, payloadJSON);
    
    if (erro || !doc.containsKey("mac_destino")) {
        _logger->erro("BRIDGE", "Ordem ignorada. O Payload não continha a chave 'mac_destino' ou era invalido.");
        return;
    }

    String macStr = doc["mac_destino"].as<String>();
    
    // Verifica se a ordem é para o próprio Hub!
    if (macStr == "M3_HUB" || macStr == WiFi.macAddress()) {
        _logger->info("BRIDGE", "Ordem recebida para o proprio HUB. Executando localmente...");
        return; 
    }

    _logger->info("BRIDGE", "Roteando para malha ESP-NOW: " + macStr);

    uint8_t macAlvo[6];
    if (extrairMACDestino(payloadJSON, macAlvo)) {
        // Envia o payload cego para a malha interna. O scDespachanteComandos do modulo alvo validará o TS (Replay Attack).
        _transceptor->enviarPacote(macAlvo, payloadJSON);
        _logger->info("BRIDGE", "Ordem atirada via radio para o alvo fisico.");
    }
}

bool scRoteadorBridgeM3::extrairMACDestino(const String& payload, uint8_t* macDestinoBytes) {
    // Usamos um doc pequeno apenas para ler o cabeçalho de roteamento
    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, payload);
    
    if (erro) return false;
    if (!doc.containsKey("mac_destino")) return false;
    
    String macStr = doc["mac_destino"].as<String>();
    
    // Converte a string "AA:BB:CC:DD:EE:FF" para array de bytes
    if (macStr.length() == 17) {
        for (int i = 0; i < 6; i++) {
            macDestinoBytes[i] = (uint8_t)strtoul(macStr.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
        }
        return true;
    }
    
    return false;
}
