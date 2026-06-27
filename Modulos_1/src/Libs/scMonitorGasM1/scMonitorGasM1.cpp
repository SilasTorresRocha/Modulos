#include "scMonitorGasM1.h"

scMonitorGasM1::scMonitorGasM1() {
    _logger = nullptr;
    _pinoAnalogico = A0;
    _limiteGas = 500; // Valor default conservador inicial
    _indiceLeitura = 0;
    _somaLeituras = 0;
    _mediaAtual = 0;
    _vazamentoDetectado = false;
    _ultimoProcessamento = 0;
    _callback = nullptr;

    for (uint8_t i = 0; i < TAMANHO_JANELA; i++) {
        _leituras[i] = 0;
    }
}

void scMonitorGasM1::inicializar(scLogger* logger, uint8_t pinoAnalogico) {
    _logger = logger;
    _pinoAnalogico = pinoAnalogico;

    if (!_logger) {
        // Princípio 10: Proibição de Serial.print. assumindo Hardware Panic local (Fail-Fast).
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            yield(); // Impede que o WDT do ESP resete imediatamente antes de notar o LED
        }
    }

    pinMode(_pinoAnalogico, INPUT);
    
    if (_logger) {
        _logger->info("GAS_M1", "Monitor de Gas inicializado no pino analógico.");
    }
}

void scMonitorGasM1::setSensibilidade(uint16_t limiteGas) {
    if (limiteGas != _limiteGas) {
        _limiteGas = limiteGas;
        if (_logger) {
            _logger->info("GAS_M1", "Limite de sensibilidade alterado via contrato para: " + String(_limiteGas));
        }
    }
}

void scMonitorGasM1::setCallbackVazamento(CallbackVazamento callback) {
    _callback = callback;
}

void scMonitorGasM1::processar() {
    // Leitura bare metal não-bloqueante, respeitando o ritmo do loop
    if (millis() - _ultimoProcessamento >= INTERVALO_LEITURA_MS) {
        _ultimoProcessamento = millis();

        // Amostragem analógica
        uint16_t novaLeitura = analogRead(_pinoAnalogico);

        // Cálculo da Média Móvel (Filtro de Ruído RF / Wi-Fi)
        _somaLeituras = _somaLeituras - _leituras[_indiceLeitura]; // Subtrai a amostra mais antiga
        _leituras[_indiceLeitura] = novaLeitura;                   // Insere a amostra nova
        _somaLeituras = _somaLeituras + _leituras[_indiceLeitura]; // Adiciona na soma total
        
        _indiceLeitura = (_indiceLeitura + 1) % TAMANHO_JANELA;    // Avança ponteiro circular
        
        _mediaAtual = _somaLeituras / TAMANHO_JANELA;              // Atualiza média filtrada

        // Análise do Ponto de Ruptura (Com Histerese Anti-Flicker)
        bool estadoAnterior = _vazamentoDetectado;
        
        // Histerese de 20 pontos evita que relés e alarmes liguem/desliguem freneticamente
        // caso a leitura flutue exatamente na linha do limite (_limiteGas).
        uint16_t margemHisterese = 20; 
        if (margemHisterese > _limiteGas) {
             margemHisterese = 0; // Proteção matemática se o usuário botar um limite bizarramente baixo
        }

        if (!_vazamentoDetectado && _mediaAtual >= _limiteGas) {
            _vazamentoDetectado = true;
        } else if (_vazamentoDetectado && _mediaAtual < (_limiteGas - margemHisterese)) {
            _vazamentoDetectado = false;
        }

        // Disparo de Evento P2P (Edge-Trigger)
        if (_vazamentoDetectado != estadoAnterior) {
            if (_vazamentoDetectado) {
                if (_logger) {
                    _logger->erro("GAS_M1", "ALERTA! Vazamento detectado! Nivel: " + String(_mediaAtual) + " >= " + String(_limiteGas));
                }
            } else {
                if (_logger) {
                    _logger->info("GAS_M1", "Situacao normalizada. Nivel de gas caiu para: " + String(_mediaAtual));
                }
            }

            // Transfere a responsabilidade da reação de rede (Anti-SPOF) para a camada superior,
            // garantindo o alto desacoplamento e qualidade  desta classe base.
            if (_callback) {
                _callback(_vazamentoDetectado, _mediaAtual);
            }
        }
    }
}

uint16_t scMonitorGasM1::obterNivelGas() const {
    return _mediaAtual;
}

bool scMonitorGasM1::VazamentoDetectado() const {
    return _vazamentoDetectado;
}
