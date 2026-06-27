#ifndef SC_MONITOR_SAUDE_M1_H
#define SC_MONITOR_SAUDE_M1_H

#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../../LibsGlobais/scSaudeHardware/scSaudeHardware.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

class scMonitorSaudeM1 : public scSaudeHardware {
public:
    scMonitorSaudeM1();
    
    // Injeção de dependências vitais de leitura (Logger, DS18B20 instanciado e pino analógico)
    void inicializarLocal(scLogger* logger, DallasTemperature* sensorTermico, uint8_t pinoAnalogico, uint8_t enderecoI2CDisplay = 0x3C);
    
    // Motor de varredura proativa. Executar no loop()
    void verificarIntegridadeLocal();
    
    // Setters de bypass caso camadas superiores (scWatchfacesM1) detectem o problema antes do ping de varredura
    void setDisplayConectado(bool conectado);
    void setGasZeroAbsoluto(bool falhaAnalogica);

    // Getters de contrato 
    bool TemErroCritico();
    String ObterUltimoErro();

private:
    scLogger* _logger;
    DallasTemperature* _sensorTermico;
    
    uint8_t _enderecoI2C;
    uint8_t _pinoAnalogico;
    
    // Flags de Saúde Física
    bool _displayDesconectado;
    bool _ds18b20Desconectado;
    bool _mq2FalhaAnalogica;
    
    // Travas de Flood de Logs (Edge-Triggering)
    bool _alertaEnviadoDisplay;
    bool _alertaEnviadoDS;
    bool _alertaEnviadoMQ2;
    
    uint32_t _ultimoCheck;
    
    // Acumulador de tolerância contra ruído rápido no ADC
    uint8_t _contagemZerosMq2;
};

#endif // SC_MONITOR_SAUDE_M1_H
