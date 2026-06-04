#include "scGestorDisplay.h"

scGestorDisplay::scGestorDisplay() : _paginaAtual(0) {
}

void scGestorDisplay::inicializar() {
    // Inicializacao do hardware do display 
}

void scGestorDisplay::atualizar() {
    // Loop de desenho nao bloqueante usando millis()
}

void scGestorDisplay::setPaginaAtual(uint8_t pagina) {
    _paginaAtual = pagina;
}

void scGestorDisplay::proximaPagina() {
    _paginaAtual++;
}

void scGestorDisplay::desenharWatchFace() {
}

void scGestorDisplay::desenharInfoCard() {
}
