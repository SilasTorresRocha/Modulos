#include "scMonitorSaudeM2.h"

#define INTERVALO_CHECK_MS 10000 // Varre o barramento I2C a cada 10 segundos

scMonitorSaudeM2::scMonitorSaudeM2() : scSaudeHardware() {
    _logger = nullptr;
    _displayDesconectado = false;
    _alertaEnviado = false;
    _ultimoCheck = 0;
    _enderecoI2C = 0x3C;
}

void scMonitorSaudeM2::inicializarLocal(scLogger* logger, uint8_t enderecoI2CDisplay) {
    _logger = logger;
    _enderecoI2C = enderecoI2CDisplay;
    
    if (!_logger) {
        // Princípio 9: O próprio logger não pode ser nulo. Se for, falha crassa de injeção.
        // Sem logger, tentamos usar Serial.println como último recurso bare metal.
        Serial.println("[CRITICO] scMonitorSaudeM2 inicializado sem injecao do scLogger!");
    }
}

void scMonitorSaudeM2::verificarIntegridadeLocal() {
    // Varredura Proativa (Non-Blocking)
    if (millis() - _ultimoCheck >= INTERVALO_CHECK_MS) {
        _ultimoCheck = millis();
        
        // Ping Físico I2C no Display OLED
        Wire.beginTransmission(_enderecoI2C);
        uint8_t erro = Wire.endTransmission();
        
        if (erro != 0) {
            // Display sumiu do barramento I2C (Solda fria, cabo rompido, curto)
            if (!_displayDesconectado) {
                _displayDesconectado = true;
                _alertaEnviado = false; // Destrava o alerta para gerar um novo log de queda
            }
        } else {
            // Display respondeu
            if (_displayDesconectado) {
                _displayDesconectado = false;
                _alertaEnviado = false; // Reset da trava
                if (_logger) {
                    _logger->info("SAUDE_M2", "Hardware OK: Display OLED I2C reconectado e responsivo.");
                }
            }
        }
        
        // Tratativa de Falha Crítica com Logger (Princípio 9 - Falhar Alto e Claro)
        if (_displayDesconectado && !_alertaEnviado) {
            if (_logger) {
                _logger->erro("SAUDE_M2", "Falha de Hardware: Display OLED (0x" + String(_enderecoI2C, HEX) + ") nao responde via I2C!");
            } else {
                Serial.println("[CRITICO] SAUDE_M2: Falha no Display I2C!");
            }
            _alertaEnviado = true; // Previne flood de mensagens no terminal (Grava apenas o Edge-Trigger)
        }
    }
}

void scMonitorSaudeM2::setDisplayConectado(bool conectado) {
    _displayDesconectado = !conectado;
}

bool scMonitorSaudeM2::temErroCritico() {
    // Retorna true se houver qualquer anomalia física na placa M2
    return _displayDesconectado;
}

String scMonitorSaudeM2::obterUltimoErro() {
    // Retorna o código exato mapeado pelo Backend
    if (_displayDesconectado) return "falha_i2c_display";
    
    return "";
}
