#include "scRelogioSincronizado.h"
#include <time.h>

scRelogioSincronizado::scRelogioSincronizado() {
    _timestampBase = 0;
    _millisSincronizacao = 0;
    _sincronizado = false;
}

void scRelogioSincronizado::inicializar(long fusoHorarioSegundos) {
    // Inicia o SNTP nativo do ESP. Ele fará a comunicacao com a porta UDP 123 completamente em background pelo OS da placa, sem travar o loop.
    configTime(fusoHorarioSegundos, 0, "pool.ntp.org", "time.nist.gov", "a.st1.ntp.br");
}

void scRelogioSincronizado::atualizar() {
    static uint32_t ultimoCheck = 0;
    uint32_t tempoAtual = millis();
    
    // Checa a API de tempo do SO apenas a cada 1 segundo para poupar ciclos
    if (tempoAtual - ultimoCheck >= 1000) {
        ultimoCheck = tempoAtual;
        
        time_t agora = time(nullptr);
        // 1600000000 = Setembro de 2020
        // Se for menor, a placa acha que esta em 1970 (NTP ainda não conectou).
        if (agora > 1600000000) {
            // Se o SNTP pescou a hora da internet, calibramos nossa base interna!
            definirHoraManualmente((uint32_t)agora);
        }
    }
}

uint32_t scRelogioSincronizado::obterHoraUnix() {
    if (!_sincronizado) return 0;
    
    // MOTOR DE EXTRAPOLAÇÃO VIA MILLIS (Software RTC):
    // Se a internet cair 1 segundo apos o boot, o SNTP vai parar de atualizar.
    // Mas esta matematica garante que o relogio continuará correndo perfeitamente, baseado nos ticks do cristal fisico da propria placa
    
    uint32_t deltaMillis = (uint32_t)(millis() - _millisSincronizacao);
    uint32_t deltaSegundos = deltaMillis / 1000;
    
    return _timestampBase + deltaSegundos;
}

void scRelogioSincronizado::definirHoraManualmente(uint32_t timestampUnix) {
    // Esse metodo é chamado ou pelo SNTP (online) ou pelo ESP-NOW (offline/Hub).
    // Ele reseta a ancora temporal do motor de extrapolação
    _timestampBase = timestampUnix;
    _millisSincronizacao = millis();
    _sincronizado = true;
}

bool scRelogioSincronizado::estaSincronizado() {
    return _sincronizado;
}
