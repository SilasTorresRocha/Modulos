#include "scRelogioSincronizado.h"

scRelogioSincronizado::scRelogioSincronizado() {
    ultimaSincronizacaoNTP = 0;
    timestampInterno = 0;
}

void scRelogioSincronizado::iniciar(long fusoHorarioSegundos) {
    // Iniciar udp e ntpClient com o fuso definido
}

unsigned long scRelogioSincronizado::obterHoraUnix() {
    // Retornar hora via NTP, e se der erro, calcular o deltaT desde o ultimo recebimento do Hub
    return timestampInterno;
}

void scRelogioSincronizado::definirHoraManualmente(unsigned long timestampUnix) {
    // Atualizar relogio interno com timestamp via ESP-NOW vindo do DS3231 do Hub
    timestampInterno = timestampUnix;
}
