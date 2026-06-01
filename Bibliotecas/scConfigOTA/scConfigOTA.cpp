#include "scConfigOTA.h"

scConfigOTA::scConfigOTA() {
    fileSystemPronto = false;
}

void scConfigOTA::iniciar() {
    // Iniciar LittleFS ou NVS
}

void scConfigOTA::iniciarServidorOTA() {
    // Configurar e rodar o servidor OTA (ArduinoOTA ou AsyncEleganceOTA)
}

void scConfigOTA::tratarOTA() {
    // Loop do OTA Handler
}

bool scConfigOTA::salvarCredenciais(const String& chave, const String& valor) {
    // Gravar no disco e retornar true/false
    return false;
}

String scConfigOTA::lerCredencial(const String& chave) {
    // Buscar chave salva ou retornar string vazia
    return "";
}
