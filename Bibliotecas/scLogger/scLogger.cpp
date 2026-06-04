#include "scLogger.h"

scLogger::scLogger() : _outputSerial(true) {
}

void scLogger::inicializar(bool outputSerial) {
    _outputSerial = outputSerial;
}

void scLogger::registrar(NivelLog nivel, String origem, String mensagem) {
    if (_outputSerial) {
        Serial.println(formatarMensagem(nivel, origem, mensagem));
    }
    // Opcional: salvar no LittleFS se for LOG_ERROR
}

String scLogger::formatarMensagem(NivelLog nivel, String origem, String mensagem) {
    String prefixo = "";
    if (nivel == LOG_INFO) prefixo = "[INFO]";
    else if (nivel == LOG_WARNING) prefixo = "[WARN]";
    else if (nivel == LOG_ERROR) prefixo = "[ERR]";

    return prefixo + " [" + origem + "] " + mensagem;
}
