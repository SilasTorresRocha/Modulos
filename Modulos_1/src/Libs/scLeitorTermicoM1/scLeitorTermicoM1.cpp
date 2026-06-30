#include "scLeitorTermicoM1.h"

// O sensor DS18B20 em resolução de 12-bits (padrão) exige ~750ms para completar uma leitura
#define TEMPO_CONVERSAO_MS 750 
// Para ter um motor de derivada termal (C°/s) liso e preciso, a amostra ocorre a cada 1 segundo
#define INTERVALO_AMOSTRAGEM_MS 1000 

scLeitorTermicoM1::scLeitorTermicoM1() {
    _logger = nullptr;
    _pinoOneWire = -1; // Pino arbitrário default. O .ino deve definir.
    _oneWire = nullptr;
    _sensorDS = nullptr;
    
    _temperaturaAtual = 0.0;
    _temperaturaAnterior = 0.0;
    _derivadaTermica = 0.0;
    
    _limiteAbsoluto = 55.0; // Ponto de ruptura absoluto (C°)
    _limiteDerivada = 0.5;  // Ponto de ruptura variação térmica (C°/s) - Meio grau por segundo
    
    _fornoLigado = false;
    _ultimoPedidoConversao = 0;
    _ultimaLeitura = 0;
    _estadoLeitura = AGUARDANDO_PEDIDO;
    _callback = nullptr;
}

void scLeitorTermicoM1::inicializar(scLogger* logger, uint8_t pinoOneWire) {
    _logger = logger;
    _pinoOneWire = pinoOneWire;

    if (!_logger) {
        // Princípio 10: Proibição de Serial.print. Assumindo Hardware Panic local (Fail-Fast).
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            yield(); // Impede reset instintivo do WDT para sinalizarmos a falha
        }
    }

    // Instanciação dinâmica (Lazy) para garantir que ocorra apenas na inicialização oficial
    _oneWire = new OneWire(_pinoOneWire);
    _sensorDS = new DallasTemperature(_oneWire);
    
    _sensorDS->begin();
    
    // A regra de ouro térmica: Nunca travar o loop esperando a conversão do sensor!
    _sensorDS->setWaitForConversion(false); 

    if (_logger) {
        _logger->info("TERM_M1", "Leitor Termico (DS18B20) inicializado em modo assíncrono.");
    }
}

void scLeitorTermicoM1::setLimiteAbsoluto(float limite) {
    if (_limiteAbsoluto != limite) {
        _limiteAbsoluto = limite;
        if (_logger) {
            _logger->info("TERM_M1", "Limite térmico absoluto alterado para: " + String(_limiteAbsoluto) + " C");
        }
    }
}

void scLeitorTermicoM1::setLimiteDerivada(float limiteDerivada) {
    if (_limiteDerivada != limiteDerivada) {
        _limiteDerivada = limiteDerivada;
        if (_logger) {
            _logger->info("TERM_M1", "Limite térmico de subida alterado para: " + String(_limiteDerivada) + " C/s");
        }
    }
}

void scLeitorTermicoM1::setCallbackEstadoForno(CallbackEstadoForno callback) {
    _callback = callback;
}

void scLeitorTermicoM1::processar() {
    // Máquina de estados para coleta de temperatura sem congelar
    if (_estadoLeitura == AGUARDANDO_PEDIDO) {
        if (millis() - _ultimaLeitura >= INTERVALO_AMOSTRAGEM_MS) {
            _sensorDS->requestTemperatures();
            _ultimoPedidoConversao = millis();
            _estadoLeitura = AGUARDANDO_CONVERSAO;
        }
    } else if (_estadoLeitura == AGUARDANDO_CONVERSAO) {
        // Só tenta ler os dados SE o chip do DS18B20 já teve tempo físico para resolver o conversor A/D
        if (millis() - _ultimoPedidoConversao >= TEMPO_CONVERSAO_MS) {
            float tempLida = _sensorDS->getTempCByIndex(0);
            
            // Se o cabo derreter ou soltar, a DallasTemperature retorna DEVICE_DISCONNECTED_C (-127).
            // Retorna ao pedido de forma limpa. Quem grita sobre isso é o scMonitorSaudeM1.
            if (tempLida == DEVICE_DISCONNECTED_C) {
                 _estadoLeitura = AGUARDANDO_PEDIDO;
                 return; 
            }

            // Motor de Derivada Térmica (C°/s) - Identifica o momento exato em que a chama/resistência ligou
            uint32_t deltaT = millis() - _ultimaLeitura;
            _ultimaLeitura = millis(); // Reset de fase para o próximo ciclo

            // Desliza a janela de dados
            _temperaturaAnterior = _temperaturaAtual;
            _temperaturaAtual = tempLida;

            // Previne falha matemática caso o loop gire sem consumir millis
            if (deltaT > 0) {
                if (_temperaturaAnterior == 0.0) {
                    // Ignora a derivada na primeira amostragem após o boot para evitar falsos "Chama Alta"
                    _derivadaTermica = 0.0;
                } else {
                    // Fator de escala * 1000 pois deltaT está em milissegundos
                    _derivadaTermica = ((_temperaturaAtual - _temperaturaAnterior) / (float)deltaT) * 1000.0;
                }
            } else {
                _derivadaTermica = 0.0;
            }

            // Máquina Lógica de Ruptura (O forno ligou?)
            bool estadoAnterior = _fornoLigado;
            
            // Histerese Térmica: A inércia de um forno faz ele continuar quente por um bom tempo após desligar
            float histerese = 3.0;

            if (!_fornoLigado) {
                // Para deduzir o Ligar: O aquecimento está rápido (derivada) OU o ambiente está perigosamente insalubre.
                if (_derivadaTermica >= _limiteDerivada || _temperaturaAtual >= _limiteAbsoluto) {
                    _fornoLigado = true;
                }
            } else {
                // Para deduzir o Desligar: 
                // A temperatura caiu ligeiramente abaixo do limite (anti-ruído de 0.5) E não está mais subindo.
                if (_temperaturaAtual < (_limiteAbsoluto - 0.5) && _derivadaTermica <= 0.05) {
                    _fornoLigado = false;
                }
            }

            // Se o motor percebeu inversão na condição da máquina
            if (_fornoLigado != estadoAnterior) {
                if (_fornoLigado) {
                    _logger->info("TERM_M1", "FORNO LIGADO! Temp Atual: " + String(_temperaturaAtual) + " C | Taxa de Subida: " + String(_derivadaTermica) + " C/s");
                } else {
                    _logger->info("TERM_M1", "FORNO DESLIGADO/RESFRIANDO. Retornou abaixo dos limites operacionais.");
                }

                // Injeta na camada superior (scGestorAlarmesM1) a mudança térmica de forma desacoplada
                if (_callback) {
                    _callback(_fornoLigado);
                }
            }

            // Reinicia a máquina de estados para ler no próximo segundo
            _estadoLeitura = AGUARDANDO_PEDIDO;
        }
    }
}

float scLeitorTermicoM1::obterTemperatura() const {
    return _temperaturaAtual;
}

float scLeitorTermicoM1::obterDerivadaTermica() const {
    return _derivadaTermica;
}

bool scLeitorTermicoM1::FornoLigado() const {
    return _fornoLigado;
}
