#ifndef SC_SAUDE_HARDWARE_H
#define SC_SAUDE_HARDWARE_H

#include <Arduino.h>

class scSaudeHardware {
public:
    scSaudeHardware();
    void inicializar(uint32_t timeoutWatchdogSegundos = 5);
    void alimentarWatchdog(); // Deve ser chamado em toda iteracao do loop principal
    
    uint32_t getRamLivre();
    uint32_t getUptimeSegundos();
    float getTemperaturaInterna();

private:
    uint32_t _uptimeInicial;
};

#endif // SC_SAUDE_HARDWARE_H
