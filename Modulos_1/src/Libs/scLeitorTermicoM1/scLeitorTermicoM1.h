#ifndef SC_LEITOR_TERMICO_M1_H
#define SC_LEITOR_TERMICO_M1_H

#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "../../LibsGlobais/scLogger/scLogger.h"

// Tipo de função callback para notificar mudanças de estado do forno (Ligado/Desligado).
// Isso desacopla a classe térmica do gestor de alarmes e contadores.
typedef void (*CallbackEstadoForno)(bool fornoLigado);

class scLeitorTermicoM1 {
public:
    scLeitorTermicoM1();

    // Injeta dependências físicas e lógicas (Logger mandatório pelo Princípio 9)
    void inicializar(scLogger* logger, uint8_t pinoOneWire);

    // Motor assíncrono termal. Deve ser invocado continuamente no super loop().
    void processar();

    // Atualiza remota/localmente o ponto cego de ruptura térmica e variação (C°/s)
    void setLimiteAbsoluto(float limite);
    void setLimiteDerivada(float limiteDerivada);

    // Registra a função de callback acionada na transição do estado do forno
    void setCallbackEstadoForno(CallbackEstadoForno callback);

    // Getters vitais para envio na Telemetria 
    float obterTemperatura() const;
    float obterDerivadaTermica() const; // Referente à chave "tx_temp" do contrato
    bool FornoLigado() const;
    DallasTemperature* obterSensorDS() const { return _sensorDS; }

private:
    scLogger* _logger;
    uint8_t _pinoOneWire;
    
    // Instâncias do barramento e sensor físico
    OneWire* _oneWire;
    DallasTemperature* _sensorDS;

    // Variáveis do motor matemático
    float _temperaturaAtual;
    float _temperaturaAnterior;
    float _derivadaTermica; // Taxa de aquecimento em °C/s

    float _limiteAbsoluto;  // Temperatura máxima que aciona o forno independentemente da derivada
    float _limiteDerivada;  // Taxa de aceleração mínima (°C/s) para caracterizar ignição do forno

    bool _fornoLigado;
    
    // Controle do timer não-bloqueante
    uint32_t _ultimoPedidoConversao;
    uint32_t _ultimaLeitura;
    
    // Máquina de estados de conversão DS18B20
    enum EstadoLeitura {
        AGUARDANDO_PEDIDO,
        AGUARDANDO_CONVERSAO
    };
    EstadoLeitura _estadoLeitura;

    CallbackEstadoForno _callback;
};

#endif // SC_LEITOR_TERMICO_M1_H
