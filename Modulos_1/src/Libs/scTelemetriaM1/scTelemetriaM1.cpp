#include "scTelemetriaM1.h"

#include "../scMonitorGasM1/scMonitorGasM1.h"
#include "../scLeitorTermicoM1/scLeitorTermicoM1.h"
#include "../scGestorAlarmesM1/scGestorAlarmesM1.h"
#include "../scMonitorSaudeM1/scMonitorSaudeM1.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

scTelemetriaM1::scTelemetriaM1() : scTelemetriaBase() {
    _gas = nullptr;
    _termico = nullptr;
    _alarmes = nullptr;
    _saudeM1 = nullptr;
    _gestorRede = nullptr;
}

void scTelemetriaM1::inicializar(scMQTTLib* mqtt, scLogger* logger, scMonitorSaudeM1* saude, 
                                 scGestorRede* gestorRede, scTransceptorESPNow* transceptor, 
                                 scMonitorGasM1* gas, scLeitorTermicoM1* termico, scGestorAlarmesM1* alarmes,
                                 const uint8_t* macHub) {
    _gas = gas;
    _termico = termico;
    _alarmes = alarmes;
    _saudeM1 = saude;
    _gestorRede = gestorRede;
    
    // Inicia a superclasse base (que já monta o cabeçalho base de tempo/RSSI e lida com o Buffer JSON)
    String macLocal = WiFi.macAddress();
    scTelemetriaBase::inicializar(mqtt, logger, saude, gestorRede, transceptor, macHub, macLocal, "M1");
}

void scTelemetriaM1::despacharTelemetriaM1() {
    // Apenas despacha se os componentes mais vitais existirem (Anti-Crash Pointer)
    if (!_gas || !_termico || !_saudeM1) return;

    // Limpa o buffer antigo e inicia um novo objeto JSON na RAM
    // A Base escreve os dados padrões automaticamente (up_s, upt_est, rssi, mac, etc.)
    iniciarPacote();
    
    // Acopla os dados de Negócio (O Contrato JSON Rigoroso do Módulo 1)
    adicionarFloat("temp", _termico->obterTemperatura(), 1);
    adicionarFloat("tx_temp", _termico->obterDerivadaTermica(), 2);
    
    // O backend espera o gas como float no contrato (apesar de ser lido como int 0-1024)
    adicionarFloat("gas", (float)_gas->obterNivelGas(), 0); 
    
    if (_alarmes) {
        adicionarInteiro("t_forno", _alarmes->TempoForno());
        adicionarInteiro("alm_prep", _alarmes->AlmPrep());
        adicionarInteiro("alm_crit", _alarmes->AlmCrit());
    }
    
    // Adicionar erros de saúde física (I2C mortos, fios cortados do MQ-2, etc.)
    if (_saudeM1->TemErroCritico()) {
        adicionarErro(_saudeM1->ObterUltimoErro()); 
    }
    
    // Executa o disparo pela scTelemetriaBase.
    // É a base que decidirá se envia isso via MQTT (nuvem) ou via ESP-NOW (Fallback pro Hub M3).
    despachar();
}
