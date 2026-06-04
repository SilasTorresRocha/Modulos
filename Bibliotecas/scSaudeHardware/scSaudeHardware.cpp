#include "scSaudeHardware.h"

scSaudeHardware::scSaudeHardware() {
}

void scSaudeHardware::inicializar(uint32_t timeoutWatchdogSegundos) {
    // Configurar o Hardware Watchdog do ESP
}

void scSaudeHardware::alimentarWatchdog() {
    // Resetar o timer do watchdog
}

uint32_t scSaudeHardware::getRamLivre() {
    return ESP.getFreeHeap();
}

uint32_t scSaudeHardware::getUptimeSegundos() {
    return millis() / 1000;
}

float scSaudeHardware::getTemperaturaInterna() {
    return 0.0; // Implementar leitura do sensor interno se disponivel
}
