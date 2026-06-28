#include "scBotaoMultifuncao.h"

scBotaoMultifuncao::scBotaoMultifuncao(uint8_t pino) : _pino(pino) {
    _cbClique = nullptr;
    _cbDuploClique = nullptr;
    _cbSegurar = nullptr;
}

void scBotaoMultifuncao::inicializar() {
    pinMode(_pino, INPUT_PULLUP);
}

void scBotaoMultifuncao::atualizar() {
    // Logica de Debounce, detecao de single click, double click e hold.
}

bool scBotaoMultifuncao::foiClicado() {
    return false; // Stub
}

bool scBotaoMultifuncao::foiPressionadoLongo() {
    return false; // Stub
}

void scBotaoMultifuncao::setAoClicar(BotaoCallback cb) {
    _cbClique = cb;
}

void scBotaoMultifuncao::setAoDuploClique(BotaoCallback cb) {
    _cbDuploClique = cb;
}

void scBotaoMultifuncao::setAoSegurar(BotaoCallback cb) {
    _cbSegurar = cb;
}
