#ifndef SC_RELOGIO_SINCRONIZADO_H
#define SC_RELOGIO_SINCRONIZADO_H

#include <Arduino.h>

class scRelogioSincronizado {
public:
    scRelogioSincronizado();
    
    // Inicia o motor SNTP nativo do ESP em background
    void inicializar(long fusoHorarioSegundos = -10800); // Default GMT-3
    
    // Roda no loop para resgatar a hora do SNTP (quando a internet conecta)
    void atualizar();
    
    // Retorna a hora atual (Independente se veio da web ou do Hub local via ESP-NOW)
    // Se retornar 0, significa que o sistema ainda está "Cego" no tempo (1970).
    uint32_t obterHoraUnix();

    // Injeção direta de hora. Permite que o Hub (Modulo 3) force uma hora proveniente de um RTC fisico (DS3231) caso a internet esteja fora.
    void definirHoraManualmente(uint32_t timestampUnix);
    
    // Retorna se o relógio tem alguma noção da realidade
    bool estaSincronizado();

private:
    uint32_t _timestampBase;       // O último timestamp UNIX validado (ex: 1718000000)
    uint32_t _millisSincronizacao; // O millis() exato do momento em que sincronizamos
    bool _sincronizado;
    long _offsetFusoHorario;
};

#endif // SC_RELOGIO_SINCRONIZADO_H
