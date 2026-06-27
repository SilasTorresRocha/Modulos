#include "scMonitorSaudeM1.h"

#define INTERVALO_CHECK_MS 10000 // Varredura proativa a cada 10s (não entupir o barramento I2C/1-Wire)

scMonitorSaudeM1::scMonitorSaudeM1() : scSaudeHardware() {
    _logger = nullptr;
    _sensorTermico = nullptr;
    _enderecoI2C = 0x3C;
    _pinoAnalogico = A0;
    
    _displayDesconectado = false;
    _ds18b20Desconectado = false;
    _mq2FalhaAnalogica = false;
    
    _alertaEnviadoDisplay = false;
    _alertaEnviadoDS = false;
    _alertaEnviadoMQ2 = false;
    
    _ultimoCheck = 0;
    _contagemZerosMq2 = 0;
}

void scMonitorSaudeM1::inicializarLocal(scLogger* logger, DallasTemperature* sensorTermico, uint8_t pinoAnalogico, uint8_t enderecoI2CDisplay) {
    _logger = logger;
    _sensorTermico = sensorTermico;
    _pinoAnalogico = pinoAnalogico;
    _enderecoI2C = enderecoI2CDisplay;

    if (!_logger) {
        // Princípio 10: Fail-fast no boot. Um médico cego não pode operar o hospital.
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            yield();
        }
    }
}

void scMonitorSaudeM1::verificarIntegridadeLocal() {
    if (millis() - _ultimoCheck >= INTERVALO_CHECK_MS) {
        _ultimoCheck = millis();
        
        // -------------------------------------------------------------
        // 1. Ping Físico I2C (Display OLED)
        // -------------------------------------------------------------
        Wire.beginTransmission(_enderecoI2C);
        uint8_t erroI2C = Wire.endTransmission();
        
        if (erroI2C != 0) {
            if (!_displayDesconectado) {
                _displayDesconectado = true;
                _alertaEnviadoDisplay = false;
            }
        } else {
            if (_displayDesconectado) {
                _displayDesconectado = false;
                _alertaEnviadoDisplay = false;
                _logger->info("SAUDE_M1", "Hardware OK: Display I2C reconectado.");
            }
        }

        if (_displayDesconectado && !_alertaEnviadoDisplay) {
            _logger->erro("SAUDE_M1", "Falha de Hardware: Display I2C (" + String(_enderecoI2C, HEX) + ") nao responde!");
            _alertaEnviadoDisplay = true;
        }

        // -------------------------------------------------------------
        // 2. Ping 1-Wire (Sensor de Fogo / DS18B20)
        // -------------------------------------------------------------
        if (_sensorTermico) {
            // Conta os chips físicos respondendo no barramento no exato momento
            uint8_t dispositivosLigados = _sensorTermico->getDeviceCount();
            
            if (dispositivosLigados == 0) {
                if (!_ds18b20Desconectado) {
                    _ds18b20Desconectado = true;
                    _alertaEnviadoDS = false;
                }
            } else {
                if (_ds18b20Desconectado) {
                    _ds18b20Desconectado = false;
                    _alertaEnviadoDS = false;
                    _logger->info("SAUDE_M1", "Hardware OK: Sensor Termico (1-Wire) voltou ao barramento.");
                }
            }

            if (_ds18b20Desconectado && !_alertaEnviadoDS) {
                _logger->erro("SAUDE_M1", "Falha de Hardware: Cabo do DS18B20 possivelmente derretido ou solto!");
                _alertaEnviadoDS = true;
            }
        }

        // -------------------------------------------------------------
        // 3. Sanidade Analógica (MQ-2)
        // -------------------------------------------------------------
        // O ar natural nunca devolve 0V num sensor MQ-2 sadio (fica entre 30~50)
        uint16_t adc = analogRead(_pinoAnalogico);
        
        if (adc == 0) {
            _contagemZerosMq2++;
            // Tolerância: Se ler 0V cravado por 30 segundos (3 checks), condena o hardware.
            if (_contagemZerosMq2 >= 3) {
                if (!_mq2FalhaAnalogica) {
                    _mq2FalhaAnalogica = true;
                    _alertaEnviadoMQ2 = false;
                }
            }
        } else {
            _contagemZerosMq2 = 0;
            if (_mq2FalhaAnalogica) {
                _mq2FalhaAnalogica = false;
                _alertaEnviadoMQ2 = false;
                _logger->info("SAUDE_M1", "Hardware OK: Pino ADC0 lendo ruidos naturais de operacao.");
            }
        }

        if (_mq2FalhaAnalogica && !_alertaEnviadoMQ2) {
            _logger->erro("SAUDE_M1", "Falha de Hardware: MQ-2 cravado em 0V constante. Modulo ou trilha rompida!");
            _alertaEnviadoMQ2 = true;
        }
    }
}

void scMonitorSaudeM1::setDisplayConectado(bool conectado) {
    _displayDesconectado = !conectado;
}

void scMonitorSaudeM1::setGasZeroAbsoluto(bool falhaAnalogica) {
    _mq2FalhaAnalogica = falhaAnalogica;
}

bool scMonitorSaudeM1::TemErroCritico() {
    return _displayDesconectado || _ds18b20Desconectado || _mq2FalhaAnalogica;
}

String scMonitorSaudeM1::ObterUltimoErro() {
    // Retorna a string contratual baseada na gravidade de risco do módulo (Gas > Fogo > UI)
    if (_mq2FalhaAnalogica) return "mq2_falha_analogica";
    if (_ds18b20Desconectado) return "ds18b20_desconectado";
    if (_displayDesconectado) return "i2c_morto";
    
    return "";
}
