#include "scGestorReleM1.h"

scGestorReleM1::scGestorReleM1() {
  _logger = nullptr;
  _pinoRele = 255;
  _logicaInvertida = false;
  _cargaLigada = false;
}

void scGestorReleM1::inicializar(scLogger *logger, uint8_t pinoRele,
                                 bool logicaInvertida) {
  _logger = logger;
  _pinoRele = pinoRele;
  _logicaInvertida = logicaInvertida;

  if (!_logger) {
    // Princípio 10: Proibição de Serial.print. Assumindo Hardware Panic local
    // (Fail-Fast).
    pinMode(LED_BUILTIN, OUTPUT);
    while (true) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(100);
      yield();
    }
  }

  if (_pinoRele != 255) {
    pinMode(_pinoRele, OUTPUT);
    _cargaLigada = false; // Garante estado seguro no boot
    atualizarHardware();

    if (_logger) {
      _logger->info(
          "RELE_M1",
          "Atuador SSR de Redundancia Anti-SPOF inicializado no pino " +
              String(_pinoRele));
    }
  } else {
    if (_logger) {
      _logger->erro("RELE_M1",
                    "Pino do Rele SSR nao definido na inicializacao!");
    }
  }
}

void scGestorReleM1::acionarEmergencia(bool ligarCarga) {
  if (_pinoRele == 255)
    return; // Prevenção de hardware (ponteiro/pino inválido)

  // Apenas atua e loga se houver mudança de estado real (evita floodar)
  if (_cargaLigada != ligarCarga) {
    _cargaLigada = ligarCarga;
    atualizarHardware();

    if (_logger) {
      if (_cargaLigada) {
        // Log classificado como erro pois denota o engajamento da redundância
        // de falha
        _logger->erro("RELE_M1",
                      "ACIONAMENTO P2P LOCAL: Carga de Seguranca LIGADA!");
      } else {
        _logger->info("RELE_M1",
                      "Emergencia mitigada. Carga de Seguranca DESLIGADA.");
      }
    }
  }
}

void scGestorReleM1::atualizarHardware() {
  if (_pinoRele == 255)
    return;

  if (_logicaInvertida) {
    digitalWrite(_pinoRele, _cargaLigada ? LOW : HIGH);
  } else {
    digitalWrite(_pinoRele, _cargaLigada ? HIGH : LOW);
  }
}

bool scGestorReleM1::CargaLigada() const { return _cargaLigada; }
