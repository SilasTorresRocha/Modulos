#include "scGestorRede.h"

scGestorRede::scGestorRede() {
    modoFallbackAtivo = false;
    ultimoPingRetorno = 0;
}

void scGestorRede::iniciar(const char* ssid, const char* password) {
    // Implementar tentativa inicial de conexao
}

void scGestorRede::atualizar() {
    // Checar conexao constantemente (internetDisponivel)
    // Invocar as funcoes privadas de fallback / reconnect
}

bool scGestorRede::estaEmFallback() {
    return modoFallbackAtivo;
}

void scGestorRede::ativarESPNow() {
    // Desligar WiFi e Iniciar modo ESP-NOW
}

void scGestorRede::escanearCanaisESPNow() {
    // Pular de canal em canal para achar o Modulo 3
}

void scGestorRede::tentarReconectarWiFi() {
    // Parar ESP-NOW brevemente para tentar roteador
}
