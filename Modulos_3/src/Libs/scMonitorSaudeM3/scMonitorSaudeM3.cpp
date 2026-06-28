#include "scMonitorSaudeM3.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif

#define INTERVALO_CHECK_MS 10000 // Varredura a cada 10s

scMonitorSaudeM3::scMonitorSaudeM3() : scSaudeHardware() {
    _logger = nullptr;
    _rtc = nullptr;
    _sdCard = nullptr;
    
    _rtcMorto = false;
    _sdMorto = false;
    _lvglTravado = false;
    
    _alertaEnviadoRTC = false;
    _alertaEnviadoSD = false;
    _alertaEnviadoLVGL = false;
    
    _ultimoCheck = 0;
}

void scMonitorSaudeM3::inicializarLocal(scLogger* logger, scRTCFisicoM3* rtc, scGestorSDCardM3* sdCard) {
    _logger = logger;
    _rtc = rtc;
    _sdCard = sdCard;

    if (!_logger) {
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); yield(); }
    }
}

void scMonitorSaudeM3::verificarIntegridadeLocal() {
    if (millis() - _ultimoCheck >= INTERVALO_CHECK_MS) {
        _ultimoCheck = millis();
        
        // 1. Diagnostico I2C (DS3231)
        if (_rtc) {
            bool falhaAtual = _rtc->FalhaComunicacao();
            if (falhaAtual && !_rtcMorto) {
                _rtcMorto = true;
                _alertaEnviadoRTC = false;
            } else if (!falhaAtual && _rtcMorto) {
                _rtcMorto = false;
                _alertaEnviadoRTC = false;
                _logger->info("SAUDE_M3", "Hardware OK: Modulo DS3231 voltou a responder no I2C.");
            }

            if (_rtcMorto && !_alertaEnviadoRTC) {
                _logger->erro("SAUDE_M3", "Falha de Hardware: Modulo I2C DS3231 morto ou desconectado!");
                _alertaEnviadoRTC = true;
            }
        }

        // 2. Diagnostico SPI (SD Card)
        if (_sdCard) {
            bool falhaSD = _sdCard->FalhaComunicacao();
            if (falhaSD && !_sdMorto) {
                _sdMorto = true;
                _alertaEnviadoSD = false;
            } else if (!falhaSD && _sdMorto) {
                _sdMorto = false;
                _alertaEnviadoSD = false;
                _logger->info("SAUDE_M3", "Hardware OK: SD CardSPI reconectado.");
            }

            if (_sdMorto && !_alertaEnviadoSD) {
                _logger->erro("SAUDE_M3", "Falha de Hardware: SD Card corrupto ou slot vazio!");
                _alertaEnviadoSD = true;
            }
        }
        
        // 3. Diagnostico Logico da Task LVGL
        if (_lvglTravado && !_alertaEnviadoLVGL) {
            _logger->erro("SAUDE_M3", "Falha Logica: FreeRTOS informa que a Task da tela (LVGL) esta em loop infinito (travada)!");
            _alertaEnviadoLVGL = true;
        } else if (!_lvglTravado && _alertaEnviadoLVGL) {
            _alertaEnviadoLVGL = false;
            _logger->info("SAUDE_M3", "Task LVGL normalizada.");
        }
    }
}

void scMonitorSaudeM3::setWatchdogLVGL(bool travado) {
    _lvglTravado = travado;
}

bool scMonitorSaudeM3::TemErroCritico() {
    return _rtcMorto || _sdMorto || _lvglTravado;
}

String scMonitorSaudeM3::ObterUltimoErro() {
    // Prioridade de envio na telemetria
    if (_lvglTravado) return "tela_lvgl_travada";
    if (_rtcMorto) return "ds3231_morto";
    if (_sdMorto) return "sd_card_erro";
    
    return "";
}
