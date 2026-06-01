#ifndef SC_RELOGIO_SINCRONIZADO_H
#define SC_RELOGIO_SINCRONIZADO_H

#include <Arduino.h>

class scRelogioSincronizado {
public:
    scRelogioSincronizado();
    
    // Configura o NTP (Se tiver internet)
    void iniciar(long fusoHorarioSegundos = -10800); // Default GMT-3
    
    // Retorna a hora atual (Independente se veio da web ou do Hub local)
    unsigned long obterHoraUnix();

    // Permite que o Hub (Modulo 3) force uma hora via ESP-NOW se offline
    void definirHoraManualmente(unsigned long timestampUnix);

private:
    unsigned long ultimaSincronizacaoNTP;
    unsigned long timestampInterno;
};

#endif // SC_RELOGIO_SINCRONIZADO_H
