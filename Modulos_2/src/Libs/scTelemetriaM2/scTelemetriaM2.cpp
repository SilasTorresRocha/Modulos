#include "scTelemetriaM2.h"

#include "../scGestorReles/scGestorReles.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"
#include "../scMonitorSaudeM2/scMonitorSaudeM2.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

scTelemetriaM2::scTelemetriaM2() : scTelemetriaBase() {
    _reles = nullptr;
    _gestorRede = nullptr;
    _saudeM2 = nullptr;
    _tempM1Cache = 0.0;
}

void scTelemetriaM2::inicializar(scMQTTLib* mqtt, scLogger* logger, scMonitorSaudeM2* saude, 
                                 scGestorRede* gestorRede, scTransceptorESPNow* transceptor, 
                                 scGestorReles* reles, const uint8_t* macHub) {
    _reles = reles;
    _gestorRede = gestorRede;
    _saudeM2 = saude;
    
    // Inicia a superclasse que gerenciará o buffer de envio (A base usara o polimorfismo e aceitara o Monitor M2!)
    String macLocal = WiFi.macAddress();
    scTelemetriaBase::inicializar(mqtt, logger, saude, gestorRede, transceptor, macHub, macLocal, "M2");
}

String scTelemetriaM2::obterIP() {
    if (_gestorRede && _gestorRede->estaEmFallback()) {
        return "Offline (ESP-NOW)";
    }
    return WiFi.localIP().toString();
}

String scTelemetriaM2::obterMAC() {
    return WiFi.macAddress();
}

String scTelemetriaM2::obterStatusRede() {
    if (_gestorRede && _gestorRede->estaEmFallback()) {
        return "ESP-NOW";
    }
    return "Wi-Fi";
}

int scTelemetriaM2::obterRSSI() {
    if (_gestorRede) {
        return _gestorRede->obterRssi();
    }
    return -100; // Ausência de sinal garantida
}

void scTelemetriaM2::setTempM1(float t) {
    _tempM1Cache = t;
}

float scTelemetriaM2::obterTempM1() {
    return _tempM1Cache;
}

void scTelemetriaM2::despacharTelemetriaM2() {
    if (!_reles || !_saudeM2) return;

    // 1. Abre a caixa do JSON e sela o cabeçalho base
    iniciarPacote();
    
    // 2. Acopla os Contratos Estritos do Módulo 2 (Status de Relé e Consumo kWh)
    adicionarBool("r1_st", _reles->getEstadoRele(1));
    adicionarBool("r2_st", _reles->getEstadoRele(2));
    
    adicionarFloat("r1_kw", _reles->getConsumoKWh(1), 3);
    adicionarFloat("r2_kw", _reles->getConsumoKWh(2), 3);
    adicionarInteiro("r1_t", _reles->getTempoLigadoSessao(1));
    adicionarInteiro("r2_t", _reles->getTempoLigadoSessao(2));
    
    float totKw = _reles->getConsumoKWh(1) + _reles->getConsumoKWh(2);
    adicionarFloat("tot_kw", totKw, 3);
    
    // 3. Adicionar erros de saúde física
    if (_saudeM2 && _saudeM2->temErroCritico()) {
        adicionarErro(_saudeM2->obterUltimoErro()); 
    }
    
    // 4. Comprime os dados e empurra para a Camada de Transporte correta (MQTT ou ESP-NOW)
    despachar();
}
