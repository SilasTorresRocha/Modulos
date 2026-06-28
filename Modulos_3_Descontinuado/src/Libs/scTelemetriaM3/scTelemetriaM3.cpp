#include "scTelemetriaM3.h"

scTelemetriaM3::scTelemetriaM3() {
    _logger = nullptr;
    _rede = nullptr;
    _relogio = nullptr;
    _rtc = nullptr;
    _monitorSaude = nullptr;
    _roteador = nullptr;
}

void scTelemetriaM3::inicializar(scLogger* logger, scGestorRede* rede, scRelogioSincronizado* relogio, scRTCFisicoM3* rtc, scMonitorSaudeM3* monitorSaude, scRoteadorBridgeM3* roteador, const String& macPlaca) {
    _logger = logger;
    _rede = rede;
    _relogio = relogio;
    _rtc = rtc;
    _monitorSaude = monitorSaude;
    _roteador = roteador;
    _macPlaca = macPlaca;
}

void scTelemetriaM3::despacharTelemetriaM3() {
    if (!_roteador) return;

    StaticJsonDocument<512> doc;

    // Assinatura do Contrato do Hub Central
    doc["mac_origem"] = _macPlaca;
    doc["tipo"] = "M3";
    doc["ts"] = _relogio ? _relogio->obterHoraUnix() : 0;
    
    // Status do Hardware Local (Diagnostico)
    JsonObject hw = doc.createNestedObject("hw");
    hw["heap"] = ESP.getFreeHeap();
    hw["uptime"] = millis() / 1000;
    
    if (_monitorSaude->TemErroCritico()) {
        hw["erro"] = _monitorSaude->ObterUltimoErro();
    } else {
        hw["erro"] = "OK";
    }

    // Leitura termal do proprio Hub
    if (_rtc && !_rtc->FalhaComunicacao()) {
        doc["dados"]["rtc_temp"] = _rtc->obterTemperaturaRTC();
    }

    // Status de Roteamento (Quais radios estao ativos no Hub)
    JsonObject net = doc.createNestedObject("net");
    net["wifi"] = _rede ? !_rede->estaEmFallback() : false;
    net["espnow"] = true; // Hub sempre esta com ESP-NOW escutando (promiscuo)

    String payload;
    serializeJson(doc, payload);

    _logger->info("TLM_M3", "Telemetria do Hub gerada. Injetando no Roteador interno...");
    
    // Delega ao Roteador. Ele fará o Bypass direto pro MQTT ou salvará no SD (JsonL) caso esteja sem internet.
    _roteador->rotearPacoteDeESPNowParaNuvem(_macPlaca.c_str(), payload.c_str());
}
