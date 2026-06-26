#ifndef SC_MONITOR_SAUDE_M2_H
#define SC_MONITOR_SAUDE_M2_H

#include <Arduino.h>
#include <Wire.h>
#include "../../LibsGlobais/scSaudeHardware/scSaudeHardware.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

class scMonitorSaudeM2 : public scSaudeHardware {
public:
    scMonitorSaudeM2();
    
    // Injeta o logger para obediência ao Princípio 9 (Não falhar silenciosamente)
    void inicializarLocal(scLogger* logger, uint8_t enderecoI2CDisplay = 0x3C);
    
    // Deve ser chamado no loop() do .ino
    // Executa varreduras fisicas não bloqueantes no hardware
    void verificarIntegridadeLocal();
    
    // Setter forçado, caso alguma classe (ex: scWatchfacesM2) capture o erro gráfico antes do ping
    void setDisplayConectado(bool conectado);
    
    // Contratos de erros exigidos pela Telemetria (scTelemetriaM2)
    bool temErroCritico();
    String obterUltimoErro();

private:
    scLogger* _logger;
    uint8_t _enderecoI2C;
    
    bool _displayDesconectado;
    bool _alertaEnviado; // Previne flood de logs no terminal
    
    uint32_t _ultimoCheck;
};

#endif // SC_MONITOR_SAUDE_M2_H
