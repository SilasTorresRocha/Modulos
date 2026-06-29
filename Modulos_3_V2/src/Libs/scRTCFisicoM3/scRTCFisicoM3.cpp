#include "scRTCFisicoM3.h"

// Dependencia da biblioteca Global para registro seguro de eventos (Zero Serial.print)
#include "../../LibsGlobais/scLogger/scLogger.h"

scRTCFisicoM3::scRTCFisicoM3() : _logger(nullptr), _ativo(false) {
}

bool scRTCFisicoM3::inicializar(scLogger* logger, TwoWire* barramentoI2C) {
    _logger = logger;
    
    // Inicia o barramento I2C do S2 nos pinos corretos definidos no PIN.txt
    Wire.begin(PINO_SDA_RTC, PINO_SCL_RTC);

    // Inicia a comunicacao I2C com o modulo usando o barramento provido (vital no ESP32-S2)
    if (!_rtc.begin(barramentoI2C)) {
        if (_logger != nullptr) {
            _logger->erro("scRTCFisicoM3", "Falha de hardware: DS3231 nao detectado no barramento I2C.");
        }
        _ativo = false;
        return false;
    }

    if (_rtc.lostPower()) {
        if (_logger != nullptr) {
            _logger->warn("scRTCFisicoM3", "Perda de energia da bateria detectada no RTC. Necessario ressincronizar com NTP.");
        }
        // Nao invalidamos o _ativo, pois ele ainda responde, apenas a data esta errada.
    }

    _ativo = true;
    if (_logger != nullptr) {
        _logger->info("scRTCFisicoM3", "RTC Fisico (DS3231) inicializado com sucesso.");
    }
    
    return true;
}

uint32_t scRTCFisicoM3::obterTimestampUnix() {
    if (!_ativo) {
        return 0; // Contrato de Falha Segura (Zero indica Hardware Morto, tratavel pela scRelogioSincronizado)
    }
    
    DateTime agora = _rtc.now();
    return agora.unixtime();
}

float scRTCFisicoM3::obterTemperatura() {
    if (!_ativo) {
        return 0.0f;
    }
    
    return _rtc.getTemperature();
}

void scRTCFisicoM3::ajustarDataHora(uint32_t timestampUnix) {
    if (!_ativo) {
        if (_logger != nullptr) {
            _logger->warn("scRTCFisicoM3", "Tentativa de ajuste de hora ignorada: Modulo RTC inativo.");
        }
        return;
    }
    
    _rtc.adjust(DateTime(timestampUnix));
    
    if (_logger != nullptr) {
        _logger->info("scRTCFisicoM3", "Hora do RTC fisico sincronizada com sucesso via timestamp.");
    }
}

bool scRTCFisicoM3::isAtivo() const {
    return _ativo;
}

String scRTCFisicoM3::obterHoraFormatada() {
    if (!_ativo) {
        return "--:--";
    }
    
    DateTime agora = _rtc.now();
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", agora.hour(), agora.minute());
    return String(buf);
}
