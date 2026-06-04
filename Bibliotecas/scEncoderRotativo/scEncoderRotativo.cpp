#include "scEncoderRotativo.h"

scEncoderRotativo* scEncoderRotativo::_instancia = nullptr;

scEncoderRotativo::scEncoderRotativo(uint8_t pinoCLK, uint8_t pinoDT) : _pinoCLK(pinoCLK), _pinoDT(pinoDT), _contagem(0) {
    _instancia = this;
}

void scEncoderRotativo::inicializar() {
    pinMode(_pinoCLK, INPUT_PULLUP);
    pinMode(_pinoDT, INPUT_PULLUP);
    
    // Atacha a ISR aos pinos de hardware nas bordas (mudanca de estado)
    attachInterrupt(digitalPinToInterrupt(_pinoCLK), isrEncoder, CHANGE);
    attachInterrupt(digitalPinToInterrupt(_pinoDT), isrEncoder, CHANGE);
}

long scEncoderRotativo::obterContagem() {
    return _contagem;
}

void scEncoderRotativo::resetarContagem() {
    _contagem = 0;
}

void IRAM_ATTR scEncoderRotativo::isrEncoder() {
    if (_instancia == nullptr) return;
    
    // Logica de verificacao rapida de quadratura para definir se incrementa ou decrementa a _contagem
}
