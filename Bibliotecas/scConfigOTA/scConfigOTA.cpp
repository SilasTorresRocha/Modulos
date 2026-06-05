#include "scConfigOTA.h"
#include "scArmazenamentoLocal.h"
#include "scLogger.h"
#include "scMQTTLib.h"

#if defined(ESP8266)
  #include <ArduinoOTA.h>
  #include <ESP8266WiFi.h>
  #include <ESP8266httpUpdate.h>
#elif defined(ESP32)
  #include <ArduinoOTA.h>
  #include <WiFi.h>
  #include <HTTPUpdate.h>
#endif

scConfigOTA::scConfigOTA() {
    _armazenamento = nullptr;
    _logger = nullptr;
    _mqtt = nullptr;
    _nomeModulo = "modulo_desconhecido";
}

void scConfigOTA::inicializar(scArmazenamentoLocal* armazenamento, scLogger* logger, scMQTTLib* mqtt, String nomeModulo) {
    _armazenamento = armazenamento;
    _logger = logger;
    _mqtt = mqtt;
    _nomeModulo = nomeModulo;

    ArduinoOTA.setHostname(_nomeModulo.c_str());
    
    // Adicionando os callbacks excelentes para logar na nuvem o status do OTA Local
    ArduinoOTA.onStart([this]() {
        if (_logger) _logger->info("OTA_LOCAL", "Iniciando atualizacao via IDE...");
    });
    
    ArduinoOTA.onEnd([this]() {
        if (_logger) _logger->info("OTA_LOCAL", "Atualizacao IDE concluida. Reiniciando...");
    });
    
    ArduinoOTA.onError([this](ota_error_t error) {
        if (_logger) _logger->erro("OTA_LOCAL", "Falha critica no OTA local. Codigo: " + String(error));
    });

    ArduinoOTA.begin();
}

void scConfigOTA::tratarLocalOTA() {
    ArduinoOTA.handle();
}

void scConfigOTA::agendarNovaURL(String novaUrl) {
    if (_armazenamento != nullptr) {
        _armazenamento->salvarChaveValor("ota_url", novaUrl);
        _armazenamento->commitarAlteracoes();
        
        if (_logger) _logger->info("OTA_HTTP", "Nova URL de firmware recebida e agendada: " + novaUrl);
        // A placa tentara fazer o download no seu proximo reboot diario as 03:00.
    }
}

void scConfigOTA::verificarAtualizacaoBoot() {
    if (_armazenamento == nullptr) return;
    
    // Checagem avancada de internet: Verifica se ha rota real pra nuvem via MQTT
    if (_mqtt != nullptr) {
        if (!_mqtt->internetDisponivel()) return;
    } else {
        // Fallback pra checagem basica caso o MQTT nao tenha sido injetado
        if (WiFi.status() != WL_CONNECTED) return;
    }
    
    String urlAlvo = _armazenamento->obterValor("ota_url");
    String urlSucesso = _armazenamento->obterValor("ota_sucesso");
    
    // PREVENÇÃO DE BOOT LOOP INFINITO
    // Diferente de usar uma URL statica padrao, usamos URLs dinamicas para travar atualizacoes repetidas
    if (urlAlvo.length() == 0 || urlAlvo == urlSucesso) {
        return; 
    }
    
    if (_logger) _logger->warn("OTA_HTTP", "Iniciando download critico de novo firmware: " + urlAlvo);
    
    WiFiClient client;
    
#if defined(ESP8266)
    ESPhttpUpdate.rebootOnUpdate(false);
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, urlAlvo);
#elif defined(ESP32)
    httpUpdate.rebootOnUpdate(false);
    t_httpUpdate_return ret = httpUpdate.update(client, urlAlvo);
#endif

    switch (ret) {
        case HTTP_UPDATE_FAILED:
            if (_logger) {
#if defined(ESP8266)
                _logger->warn("OTA_HTTP", "Falha no Download OTA. Erro: " + ESPhttpUpdate.getLastErrorString());
#elif defined(ESP32)
                _logger->warn("OTA_HTTP", "Falha no Download OTA. Erro: " + httpUpdate.getLastErrorString());
#endif
            }
            break;

        case HTTP_UPDATE_NO_UPDATES:
            if (_logger) _logger->info("OTA_HTTP", "Nenhuma atualizacao encontrada.");
            break;

        case HTTP_UPDATE_OK:
            if (_logger) _logger->info("OTA_HTTP", "Download concluido com sucesso! Reiniciando...");
            // Salva fisicamente que essa URL especifica ja esta instalada para barrar o boot loop
            _armazenamento->salvarChaveValor("ota_sucesso", urlAlvo);
            _armazenamento->commitarAlteracoes();
            delay(1000); // Tempo respiratorio para gravacao no disco
#if defined(ESP8266)
            ESP.restart();
#elif defined(ESP32)
            ESP.restart();
#endif
            break;
    }
}
