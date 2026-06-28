#include "scRTCFisicoM3.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif

scRTCFisicoM3::scRTCFisicoM3() {
    _logger = nullptr;
    _relogioGlobal = nullptr;
    _rtcConectado = false;
    _ultimoCheckCalibracao = 0;
}

void scRTCFisicoM3::inicializar(scLogger* logger, scRelogioSincronizado* relogioGlobal) {
    _logger = logger;
    _relogioGlobal = relogioGlobal;

    if (!_logger) {
        // Princípio 10: Fail-fast no boot.
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            yield();
        }
    }

    _logger->info("RTC_M3", "Iniciando barramento DS3231...");

    // Se o begin falhar, o hardware esta morto ou desconectado
    if (!_rtc.begin()) {
        _rtcConectado = false;
        _logger->erro("RTC_M3", "FALHA CRITICA: Modulo DS3231 nao encontrado no I2C!");
    } else {
        _rtcConectado = true;
        
        if (_rtc.lostPower()) {
            _logger->warn("RTC_M3", "Bateria moeda do DS3231 esgotada ou recem trocada. Hora corrompida.");
            // Nao alimentamos o scRelogioSincronizado porque a hora aqui é invalida (ex: 1970 ou 2000)
            // Aguardaremos a internet voltar para o NTP sobrescrever isso.
        } else {
            _logger->info("RTC_M3", "DS3231 respondendo. Lendo hora local offline da bateria...");
            DateTime agoraFisico = _rtc.now();
            
            // Injeta fisicamente a hora na memoria do ecossistema. 
            // Se a internet não estiver conectada, toda a casa viverá sob esse tempo!
            if (_relogioGlobal) {
                _relogioGlobal->definirHoraManualmente(agoraFisico.unixtime());
                _logger->info("RTC_M3", "Hora injetada no Sistema Global com sucesso.");
            }
        }
    }
}

void scRTCFisicoM3::atualizar() {
    if (!_rtcConectado || !_relogioGlobal) return;

    // A cada 1 hora, verificamos o motor principal
    if (millis() - _ultimoCheckCalibracao >= INTERVALO_CALIBRACAO_MS || _ultimoCheckCalibracao == 0) {
        _ultimoCheckCalibracao = millis();

        // O scRelogioSincronizado se baseia no configTime (NTP do ESP32).
        // Se a internet estiver funcionando e ja sincronizou o ESP-S2, a _timestampBase será atual atual
        if (_relogioGlobal->estaSincronizado()) {
            
            uint32_t horaNTPUnix = _relogioGlobal->obterHoraUnix();
            DateTime horaRTC = _rtc.now();
            
            // Verifica a diferenca (drift do cristal interno do DS3231 que atrasa/adianta 1min por ano)
            int32_t diff = (int32_t)horaNTPUnix - (int32_t)horaRTC.unixtime();
            
            if (abs(diff) > 2) { // Tolerancia de 2 segundos de latencia I2C/NTP
                _logger->info("RTC_M3", "Calibrando precisao DS3231 com o NTP da Nuvem (Drift: " + String(diff) + "s)");
                _rtc.adjust(DateTime(horaNTPUnix));
            }
        }
    }
}

float scRTCFisicoM3::obterTemperaturaRTC() {
    if (_rtcConectado) {
        return _rtc.getTemperature();
    }
    return -127.0f; // Erro padrão 1-Wire/Termal
}

bool scRTCFisicoM3::FalhaComunicacao() const {
    return !_rtcConectado;
}
