#ifndef SC_MONITOR_GAS_M1_H
#define SC_MONITOR_GAS_M1_H

#include <Arduino.h>
#include "../../LibsGlobais/scLogger/scLogger.h"

// Tipo de função callback para notificar mudanças no estado do gás.
// Permite que o Main (.ino) atire o comando ESP-NOW para o M2 sem acoplar esta classe ao rádio.
typedef void (*CallbackVazamento)(bool detectado, uint16_t nivelAtual);

class scMonitorGasM1 {
public:
    scMonitorGasM1();

    // Injeta o logger (Princípio 9 - Não falhar silenciosamente) e configura o pino analógico.
    void inicializar(scLogger* logger, uint8_t pinoAnalogico = A0);

    // Recebe e atualiza o limite de disparo (sens_gas vindo das configurações MQTT).
    void setSensibilidade(uint16_t limiteGas);

    // Registra a função de callback acionada na transição de estado.
    void setCallbackVazamento(CallbackVazamento callback);

    // Processamento bare metal não-bloqueante. Deve ser invocado no loop().
    void processar();

    // Getters para a Telemetria
    uint16_t obterNivelGas() const;
    bool VazamentoDetectado() const;

private:
    scLogger* _logger;
    uint8_t _pinoAnalogico;
    uint16_t _limiteGas;
    
    // Parâmetros do Filtro de Média Móvel Circular
    static const uint8_t TAMANHO_JANELA = 10;
    uint16_t _leituras[TAMANHO_JANELA];
    uint8_t _indiceLeitura;
    uint32_t _somaLeituras;
    uint16_t _mediaAtual;

    bool _vazamentoDetectado;
    
    uint32_t _ultimoProcessamento;
    static const uint32_t INTERVALO_LEITURA_MS = 50; // Amostragem a cada 50ms (janela renovada a cada 500ms)

    CallbackVazamento _callback;
};

#endif // SC_MONITOR_GAS_M1_H
