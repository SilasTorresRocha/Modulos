#include "scAvisosSonoros.h"

scAvisosSonoros::scAvisosSonoros(uint8_t pinoBuzzer)
    : _pino(pinoBuzzer), _estadoAtual(SILENCIO) {}

void scAvisosSonoros::inicializar() {
  pinMode(_pino, OUTPUT);
  digitalWrite(_pino, LOW);
}

void scAvisosSonoros::tocar(PadraoSom padrao) {
  _estadoAtual = padrao;
  // Logica de setup do som
}

void scAvisosSonoros::atualizar() {
  // Processamento nao bloqueante com millis()
}
