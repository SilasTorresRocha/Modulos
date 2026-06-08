#include "scAvisosSonoros.h"

scAvisosSonoros::scAvisosSonoros(uint8_t pinoBuzzer) {
  _pino = pinoBuzzer;
  _padraoAtual = SILENCIO;
  _ultimoTempo = 0;
  _passoAtual = 0;
  _mutado = false;
}

void scAvisosSonoros::setMute(bool mutado) {
  _mutado = mutado;
  // Se o modo mute foi ativado e tem um bip tocando (que nao seja a sirene
  // critica), cala imediatamente
  if (_mutado && _padraoAtual != SIRENE_EMERGENCIA) {
    tocar(SILENCIO);
  }
}

bool scAvisosSonoros::Mutado() {
    return _mutado;
 }

void scAvisosSonoros::inicializar() {
  pinMode(_pino, OUTPUT);
  definirPino(false);
}

void scAvisosSonoros::definirPino(bool ligado, unsigned int frequencia) {
  // Tone() padrão do Arduino que funciona nativamente no ESP8266 e nos
  // Cores recentes do ESP32.
  if (ligado) {
    tone(_pino, frequencia);
  } else {
    noTone(_pino);
  }
}

void scAvisosSonoros::tocar(PadraoSom padrao) {
  // BLINDAGEM DO MODO SILENCIOSO (Conforto do Usuario):
  // Ignora solenemente os bips cotidianos de menu/timers se o usuario mutou.
  // POREM, um vazamento de gas/hardware em chamas e mais importante que o
  // conforto. A SIRENE_EMERGENCIA "fura" o bloqueio do mute.
  if (_mutado && padrao != SIRENE_EMERGENCIA && padrao != SILENCIO) {
    return;
  }

  _padraoAtual = padrao;
  _passoAtual = 0;
  _ultimoTempo = millis();

  // Start imediato e escolha de notas musicais/frequencias basicas
  switch (_padraoAtual) {
  case SILENCIO:
    definirPino(false);
    break;

  case BIP_CURTO:
  case BIP_DUPLO:
  case ALARME_TEMPORIZADOR:
    definirPino(true, 2000); // 2kHz - Som agudo e afiado
    break;

  case BIP_LONGO:
    definirPino(true,
                1200); // 1.2kHz - Som mais grave para denotar "Gravado/Sucesso"
    break;

  case SIRENE_EMERGENCIA:
    // Sirene será uma ambulancia, entao comecamos com um tom baixo
    definirPino(true, 800);
    break;
  }
}

void scAvisosSonoros::atualizar() {
  // Filtro rapido de otimizacao:
  // O unico padrao que "fica estatico pra sempre" sem pulsar nada eo SILENCIO.
  // economiza CPU pulando fora aqui
  if (_padraoAtual == SILENCIO) {
    return;
  }

  uint32_t tempoAtual = millis();
  uint32_t tempoDecorrido = (uint32_t)(tempoAtual - _ultimoTempo);

  // Maquina de Estados
  switch (_padraoAtual) {
  case BIP_CURTO:
    if (_passoAtual == 0 && tempoDecorrido >= 100) {
      definirPino(false);
      _padraoAtual = SILENCIO; // Auto-Destruicao
    }
    break;

  case BIP_LONGO:
    if (_passoAtual == 0 && tempoDecorrido >= 600) {
      definirPino(false);
      _padraoAtual = SILENCIO;
    }
    break;

  case BIP_DUPLO:
    if (_passoAtual == 0 && tempoDecorrido >= 80) {
      definirPino(false);
      _passoAtual = 1;
      _ultimoTempo = tempoAtual;
    } else if (_passoAtual == 1 && tempoDecorrido >= 80) {
      definirPino(true, 2000);
      _passoAtual = 2;
      _ultimoTempo = tempoAtual;
    } else if (_passoAtual == 2 && tempoDecorrido >= 80) {
      definirPino(false);
      _padraoAtual = SILENCIO;
    }
    break;

  case SIRENE_EMERGENCIA:
    // Sirene de Ambulancia (Loop Infinito): Alterna tons a cada 400ms
    if (tempoDecorrido >= 400) {
      _passoAtual = (_passoAtual == 0) ? 1 : 0; // Inverte o estado

      // Se passo = 0, toca 800Hz. Se passo = 1, toca 1500Hz (Bipolaridade
      // estridente)
      definirPino(true, _passoAtual == 0 ? 800 : 1500);
      _ultimoTempo = tempoAtual;
    }
    break;

  case ALARME_TEMPORIZADOR:
    // Despertador (Loop Infinito): BipCurto - Pausa - BipCurto - Pausa Longa
    if (_passoAtual == 0 && tempoDecorrido >= 100) {
      definirPino(false);
      _passoAtual = 1;
      _ultimoTempo = tempoAtual;
    } else if (_passoAtual == 1 && tempoDecorrido >= 100) {
      definirPino(true, 2000);
      _passoAtual = 2;
      _ultimoTempo = tempoAtual;
    } else if (_passoAtual == 2 && tempoDecorrido >= 100) {
      definirPino(false);
      _passoAtual = 3;
      _ultimoTempo = tempoAtual;
    } else if (_passoAtual == 3 && tempoDecorrido >= 800) {
      definirPino(true, 2000);
      _passoAtual = 0; // REINICIA O LOOP
      _ultimoTempo = tempoAtual;
    }
    break;

  default:
    break;
  }
}
