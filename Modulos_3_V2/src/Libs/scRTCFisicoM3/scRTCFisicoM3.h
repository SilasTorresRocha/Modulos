#ifndef SC_RTC_FISICO_M3_H
#define SC_RTC_FISICO_M3_H

#include <Arduino.h>
#include <Wire.h>
#include <RTClib.h> // Dependencia: Adafruit RTClib

// Forward declaration para injecao de dependencia
class scLogger;

#define PINO_SDA_RTC 8
#define PINO_SCL_RTC 9

class scRTCFisicoM3 {
private:
    scLogger* _logger;
    RTC_DS3231 _rtc;
    bool _ativo;

public:
    scRTCFisicoM3();

    // Inicializa comunicacao I2C. Recebe o barramento para dar suporte nativo ao ESP32-S2
    bool inicializar(scLogger* logger, TwoWire* barramentoI2C = &Wire);

    // Retorna Unix Timestamp (Segundos desde 1970). Se houver falha de HW, retorna 0.
    uint32_t obterTimestampUnix();

    // Retorna a temperatura lida pelo sensor interno do chip DS3231. Se houver falha, retorna 0.0f.
    float obterTemperatura();

    // Permite sincronizar o RTC Fisico caso ele esteja atrasado (geralmente recebendo do NTP/Nuvem)
    void ajustarDataHora(uint32_t timestampUnix);

    // Retorna se o modulo fisico esta operante
    bool isAtivo() const;

    // Retorna a hora atual formatada (HH:MM)
    String obterHoraFormatada();
};

#endif // SC_RTC_FISICO_M3_H
