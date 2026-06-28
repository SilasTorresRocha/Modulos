#ifndef SC_RTC_FISICO_M3_H
#define SC_RTC_FISICO_M3_H

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"

class scRTCFisicoM3 {
public:
    scRTCFisicoM3();

    // Inicializa o DS3231 via I2C e tenta ler a hora de backup da bateria
    void inicializar(scLogger* logger, scRelogioSincronizado* relogioGlobal);

    // Motor de calibração que verifica se o NTP da internet deve corrigir o DS3231, ou se o DS3231 deve salvar o ecossistema offline
    void atualizar();

    // Getter para a temperatura ambiente interna (o DS3231 possui um sensor termal para calibrar o cristal)
    float obterTemperaturaRTC();

    // Getter para diagnóstico de hardware
    bool FalhaComunicacao() const;

private:
    scLogger* _logger;
    scRelogioSincronizado* _relogioGlobal;
    RTC_DS3231 _rtc;
    
    bool _rtcConectado;
    uint32_t _ultimoCheckCalibracao;
    
    // Intervalo para tentar sincronizar o DS3231 com a web (NTP)
    const uint32_t INTERVALO_CALIBRACAO_MS = 3600000; // 1 hora
};

#endif // SC_RTC_FISICO_M3_H
