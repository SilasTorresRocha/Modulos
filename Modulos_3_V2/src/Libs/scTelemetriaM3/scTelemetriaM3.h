#ifndef SC_TELEMETRIA_M3_H
#define SC_TELEMETRIA_M3_H

#include <Arduino.h>
#include <ArduinoJson.h>

class scLogger;
class scRTCFisicoM3;
class scMonitorSaudeM3;
class scGestorSDCardM3;
class scMQTTLib;

class scTelemetriaM3 {
private:
    scLogger* _logger;
    scRTCFisicoM3* _rtc;
    scMonitorSaudeM3* _saude;
    scGestorSDCardM3* _sd;
    scMQTTLib* _mqtt;
    
    const char* _meuMac;
    uint32_t _uptimeEmissao;

public:
    scTelemetriaM3();

    // Injeta os sensores passivos e a Caixa Preta (SD)
    void inicializar(const char* meuMac, scLogger* logger, scRTCFisicoM3* rtc, scMonitorSaudeM3* saude, scGestorSDCardM3* sd, scMQTTLib* mqtt);

    // O Agendador deve chamar este método a cada X minutos (ex: a cada 5 min)
    // Gera o pacote JSON local do Hub e joga para a fila Idempotente do SD.
    void registrarCena();
};

#endif // SC_TELEMETRIA_M3_H
