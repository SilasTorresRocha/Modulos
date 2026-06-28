#include "scEncoderRotativo.h"

scEncoderRotativo* scEncoderRotativo::_instancia = nullptr;

scEncoderRotativo::scEncoderRotativo(uint8_t pinoCLK, uint8_t pinoDT) {
    _pinoCLK = pinoCLK;
    _pinoDT = pinoDT;
    _contagem = 0;
    _estadoAnterior = 0;
    _instancia = this; // Salva o ponteiro global para a ISR enxergar
}

void scEncoderRotativo::inicializar() {
    pinMode(_pinoCLK, INPUT_PULLUP);
    pinMode(_pinoDT, INPUT_PULLUP);
    
    // Leitura do estado inicial (CLK na posicao 1, DT na posicao 0)
    uint8_t clk = digitalRead(_pinoCLK);
    uint8_t dt = digitalRead(_pinoDT);
    _estadoAnterior = (clk << 1) | dt;
    
    // Engata a interrupcao: Qualquer transicao (CHANGE) nos dois pinos congela o CPU e roda a ISR
    attachInterrupt(digitalPinToInterrupt(_pinoCLK), isrEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_pinoDT), isrEncoder, CHANGE);
}

long scEncoderRotativo::obterContagem() {
    // Evita que a interrupcao escreva na variavel no exato microsegundo
    // que o loop() do Arduino está tentando lê-la (Race Condition).
    noInterrupts(); 
    long copiaSegura = _contagem;
    interrupts();
    
    return copiaSegura;
}

void scEncoderRotativo::resetarContagem() {
    noInterrupts();
    _contagem = 0;
    interrupts();
}

int scEncoderRotativo::getDelta() {
    long atual = obterContagem();
    if (atual != 0) {
        resetarContagem();
    }
    return (int)atual;
}

// ====================================================================
// ISR (Interrupt Service Routine)
// Roda fora do loop. Deve ser a coisa mais rapida do universo
// ====================================================================
void IRAM_ATTR scEncoderRotativo::isrEncoder() {
    if (_instancia == nullptr) return;
    
    // Le os pinos instantaneamente
    uint8_t clk = digitalRead(_instancia->_pinoCLK);
    uint8_t dt = digitalRead(_instancia->_pinoDT);
    
    // Funde num par de bits de estado atual
    uint8_t estadoAtual = (clk << 1) | dt;
    
    // Monta uma matriz de transicao: (Estado Antigo + Estado Atual)
    uint8_t transicao = (_instancia->_estadoAnterior << 2) | estadoAtual;
    
    // Decodificacao por Maquina de Estados 
    // Essa matematica brutal ignora ruidos elétricos bouncing pois pulos malucos vao cair em combinacoes nao mapeadas abaixo.
    
    if (transicao == 1 || transicao == 7 || transicao == 14 || transicao == 8) {
        // Giro horario
        _instancia->_contagem++;
    } 
    else if (transicao == 2 || transicao == 11 || transicao == 13 || transicao == 4) {
        // Giro anti-horario
        _instancia->_contagem--;
    }
    
    // Salva para a proxima transicao
    _instancia->_estadoAnterior = estadoAtual;
}
