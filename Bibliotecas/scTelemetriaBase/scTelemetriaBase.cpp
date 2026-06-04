#include "scTelemetriaBase.h"

scTelemetriaBase::scTelemetriaBase() {
}

void scTelemetriaBase::inicializar() {
    // Inicialização
}

void scTelemetriaBase::construirHeartbeatBase() {
    // IMPORTANTE: Criar JsonDocument LOCALMENTE e serializar na variavel String _jsonAtual
    // Evita consumo desnecessario de Heap no ESP8266.
    // Insere mac, status de rede e ram livre
}

void scTelemetriaBase::adicionarDado(String chave, String valor) {
    // Adiciona ao objeto JSON
}

void scTelemetriaBase::enviar() {
    // Envia o JSON pro MQTT
}
