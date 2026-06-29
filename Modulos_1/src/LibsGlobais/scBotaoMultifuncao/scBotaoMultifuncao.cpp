#include "scBotaoMultifuncao.h"

scBotaoMultifuncao::scBotaoMultifuncao(uint8_t pino) : _pino(pino) {
    _cbClique = nullptr;
    _cbDuploClique = nullptr;
    _cbSegurar = nullptr;
    
    _estadoAnterior = HIGH;
    _estadoEstavel = HIGH;
    _ultimoDebounceTime = 0;
    _tempoPressionado = 0;
    _clicado = false;
    _pressionadoLongo = false;
    _acaoLongoExecutada = false;
}

void scBotaoMultifuncao::inicializar() {
    pinMode(_pino, INPUT_PULLUP);
}

void scBotaoMultifuncao::atualizar() {
    bool leituraAtual = digitalRead(_pino); // Lógica Negativa (INPUT_PULLUP)
    
    if (leituraAtual != _estadoAnterior) {
        _ultimoDebounceTime = millis();
    }
    
    if ((millis() - _ultimoDebounceTime) > 50) { // 50ms de Debounce
        if (leituraAtual != _estadoEstavel) {
            _estadoEstavel = leituraAtual;
            
            if (_estadoEstavel == LOW) { 
                // Botão foi fisicamente pressionado agora
                _tempoPressionado = millis();
                _acaoLongoExecutada = false; 
            } else { 
                // Botão foi solto
                if (!_acaoLongoExecutada) { // Se não foi segurado por mto tempo, conta como clique simples
                    _clicado = true;
                    if (_cbClique) _cbClique();
                }
            }
        }
    }
    
    // Verifica pressão longa contínua
    if (_estadoEstavel == LOW && !_acaoLongoExecutada) {
        if ((millis() - _tempoPressionado) > 800) { // 800ms = Hold
            _pressionadoLongo = true;
            _acaoLongoExecutada = true; // Impede que dispare repetidamente
            if (_cbSegurar) _cbSegurar();
        }
    }
    
    _estadoAnterior = leituraAtual;
}

bool scBotaoMultifuncao::foiClicado() {
    if (_clicado) { _clicado = false; return true; }
    return false;
}

bool scBotaoMultifuncao::foiPressionadoLongo() {
    if (_pressionadoLongo) { _pressionadoLongo = false; return true; }
    return false;
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
