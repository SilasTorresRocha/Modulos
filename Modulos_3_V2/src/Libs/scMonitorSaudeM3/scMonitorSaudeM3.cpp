#include "scMonitorSaudeM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../scGestorSDCardM3/scGestorSDCardM3.h"
#include "../scGestorBuzzerM3/scGestorBuzzerM3.h"

scMonitorSaudeM3::scMonitorSaudeM3() : 
    _logger(nullptr), _rtc(nullptr), _sd(nullptr), _buzzer(nullptr),
    _inicioLoopGrafico(0), _alertaMemoriaDisparado(false), 
    _alertaI2CDisparado(false), _alertaSPIDisparado(false) {
}

void scMonitorSaudeM3::inicializar(scLogger* logger, scRTCFisicoM3* rtc, scGestorSDCardM3* sd, scGestorBuzzerM3* buzzer) {
    _logger = logger;
    _rtc = rtc;
    _sd = sd;
    _buzzer = buzzer;

    _saudeGlobal.inicializar(5); // Armador do cão de guarda (Watchdog de 5 segundos)
    
    if (_logger != nullptr) {
        _logger->info("scMonitorSaude", "Monitoramento Local M3 ativo (Watchdog + Tolerancia a Falhas).");
    }
}

void scMonitorSaudeM3::processar() {
    // 1. Alimenta o cão de guarda físico do ESP32 para nao resetar
    _saudeGlobal.alimentarWatchdog();

    // 2. Checagem de Memoria (Falha Critica abaixo de 30KB)
    uint32_t ramLivre = _saudeGlobal.getRamLivre();
    if (ramLivre < 30000 && !_alertaMemoriaDisparado) {
        _alertaMemoriaDisparado = true;
        if (_logger != nullptr) _logger->erro("scMonitorSaude", "Vazamento de memoria ou sobrecarga (Heap < 30KB)!");
        if (_buzzer != nullptr) _buzzer->tocarSireneEmergencia();
    } else if (ramLivre >= 35000) {
        _alertaMemoriaDisparado = false; // Sistema se recuperou (Histerese)
    }

    // 3. Checagem do Barramento I2C (RTC)
    if (_rtc != nullptr && !_rtc->isAtivo() && !_alertaI2CDisparado) {
        _alertaI2CDisparado = true;
        if (_logger != nullptr) _logger->erro("scMonitorSaude", "Barramento I2C travado ou DS3231 morto.");
    }

    // 4. Checagem do Barramento SPI (SD Card)
    if (_sd != nullptr && !_sd->isAtivo() && !_alertaSPIDisparado) {
        _alertaSPIDisparado = true;
        if (_logger != nullptr) _logger->erro("scMonitorSaude", "Barramento SPI (SD) inoperante. Falha de HW grave.");
    }
}

void scMonitorSaudeM3::iniciarMedicaoLoopGrafico() {
    _inicioLoopGrafico = millis();
}

void scMonitorSaudeM3::finalizarMedicaoLoopGrafico() {
    uint32_t tempoGasto = millis() - _inicioLoopGrafico;
    
    // Se o motor grafico (LVGL) gastar mais de 200ms em uma tarefa, o sistema engasgou
    if (tempoGasto > 200) {
        if (_logger != nullptr) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Engasgo no LVGL: %lu ms. Risco de queda do Loop.", (unsigned long)tempoGasto);
            _logger->warn("scMonitorSaude", buf);
        }
    }
}

bool scMonitorSaudeM3::isCritico() {
    return _alertaMemoriaDisparado || _alertaI2CDisparado || _alertaSPIDisparado;
}

void scMonitorSaudeM3::preencherTelemetria(char* bufferSaida, size_t maxLen) {
    // Devolve um payload seguro sem usar String, cravado pra acoplar na telemetria maior
    snprintf(bufferSaida, maxLen, 
             "\"ram_livre\":%lu,\"i2c_ok\":%d,\"spi_ok\":%d,\"critico\":%d", 
             (unsigned long)_saudeGlobal.getRamLivre(),
             (_rtc != nullptr && _rtc->isAtivo()) ? 1 : 0,
             (_sd != nullptr && _sd->isAtivo()) ? 1 : 0,
             isCritico() ? 1 : 0);
             
    bufferSaida[maxLen - 1] = '\0'; // Seguranca do C
}
