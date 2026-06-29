#include "scTelemetriaM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../scMonitorSaudeM3/scMonitorSaudeM3.h"
#include "../scGestorSDCardM3/scGestorSDCardM3.h"

scTelemetriaM3::scTelemetriaM3() : 
    _logger(nullptr), _rtc(nullptr), _saude(nullptr), _sd(nullptr), _mqtt(nullptr), _meuMac(""), _uptimeEmissao(0) {
}

void scTelemetriaM3::inicializar(const char* meuMac, scLogger* logger, scRTCFisicoM3* rtc, scMonitorSaudeM3* saude, scGestorSDCardM3* sd, scMQTTLib* mqtt) {
    _meuMac = meuMac;
    _logger = logger;
    _rtc = rtc;
    _saude = saude;
    _sd = sd;
    _mqtt = mqtt;

    if (_logger != nullptr) {
        _logger->info("scTelemetria", "Empacotador de Telemetria ativo. Gerador de pacotes pronto.");
    }
}

void scTelemetriaM3::registrarCena() {
    _uptimeEmissao++;

    // Memoria estatica na Stack da Thread (Zero alocacao na Heap).
    // 512 bytes sao suficientes para um pacote de saude basico.
    StaticJsonDocument<512> doc;
    
    // ==========================================
    // 1. Metadados de Roteamento Nuvem
    // ==========================================
    doc["mac_origem"] = _meuMac;
    doc["tipo"] = "TELEMETRIA_HUB"; // Assinatura exclusiva do Modulo 3
    
    // ==========================================
    // 2. Assinatura Temporal 
    // ==========================================
    if (_rtc != nullptr) {
        doc["timestamp"] = _rtc->obterTimestampUnix();
        doc["temperatura_rtc"] = _rtc->obterTemperatura();
    } else {
        doc["timestamp"] = 0; // Fail-Fast
    }

    doc["uptime_ciclos"] = _uptimeEmissao;

    // ==========================================
    // 3. Status de Saude (Hardware Interno)
    // ==========================================
    if (_saude != nullptr) {
        // Resgata os parametros vitais crus (string no formato "chave":valor,"chave2":valor)
        char bufSaude[128];
        _saude->preencherTelemetria(bufSaude, sizeof(bufSaude));
        
        // Magica do Bare-Metal: Envolve com chaves para formar um JsonObject válido
        // sem precisar fazer parse ou instanciar copias na memoria dinâmica
        char jsonValido[150];
        snprintf(jsonValido, sizeof(jsonValido), "{%s}", bufSaude);
        
        doc["saude_hw"] = serialized(jsonValido); 
    }

    // ==========================================
    // 4. Minificacao e Escoamento
    // ==========================================
    char bufferSaida[512];
    size_t tamanhoReal = serializeJson(doc, bufferSaida, sizeof(bufferSaida));

    if (tamanhoReal == 0) {
        if (_logger != nullptr) _logger->erro("scTelemetria", "Falha critica (Buffer Overflow) ao empacotar JSON.");
        return;
    }

    // Envia fisicamente o char array pro final da fila Idempotente.
    // Zero concorrencia: quem cuida da logistica pro MQTT ou ESP-NOW agora eh o Roteador.
    if (_sd != nullptr && _sd->isAtivo()) {
        if (_sd->enfileirarOffline(bufferSaida)) {
            if (_logger != nullptr) _logger->info("scTelemetria", "Cena capturada e apensada na Fila (SD Card).");
        }
    } else {
        // [MODO LIVE / FAILBACK]: Sem SD Card disponivel! Dispara direto para o ar se tiver rede.
        if (_mqtt != nullptr) { // O _mqtt->enviarJSON ja tem protecao interna para verificar se esta conectado
            if (_mqtt->enviarJSON(bufferSaida)) {
                if (_logger != nullptr) _logger->info("scTelemetria", "Cena capturada e enviada DIRETAMENTE ao MQTT (Sem SD Card).");
            }
        }
    }
}
