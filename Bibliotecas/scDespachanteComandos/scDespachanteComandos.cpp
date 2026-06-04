#include "scDespachanteComandos.h"

scDespachanteComandos::scDespachanteComandos() {
    _gestorRede = nullptr;
    _configOTA = nullptr;
}

void scDespachanteComandos::inicializar(scGestorRede* gestorRede, scConfigOTA* configOTA) {
    _gestorRede = gestorRede;
    _configOTA = configOTA;
}

void scDespachanteComandos::processarMensagem(String payloadJson) {
    // IMPORTANTE: Criar o JsonDocument LOCALMENTE para evitar fragmentacao de RAM no ESP8266.
    // O espaco sera destruido automaticamente ao sair desta funcao.
    // StaticJsonDocument<256> doc; ou JsonDocument doc; (v7)
    
    // Logica para desserializar JSON e rotear
}

void scDespachanteComandos::resolverComandoUniversal(String comando) {
    // Tratar comandos como restart, update_wifi, etc.
}
