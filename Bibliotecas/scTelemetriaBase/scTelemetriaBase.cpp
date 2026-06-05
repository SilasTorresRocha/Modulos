#include "scTelemetriaBase.h"
#include "scMQTTLib.h"
#include "scLogger.h"
#include "scSaudeHardware.h"

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#endif

scTelemetriaBase::scTelemetriaBase() {
    _mqtt = nullptr;
    _logger = nullptr;
    _saude = nullptr;
}

void scTelemetriaBase::inicializar(scMQTTLib* mqtt, scLogger* logger, scSaudeHardware* saude, String macOrigem, String tipoModulo) {
    _mqtt = mqtt;
    _logger = logger;
    _saude = saude;
    _macOrigem = macOrigem;
    _tipoModulo = tipoModulo;
    
    // PREVENCAO DE FRAGMENTACAO DE HEAP:
    // Reserva um bloco contiguo de 300 bytes para a String no inicio da vida da placa.
    // Assim, as futuras operacoes de += reaproveitarao esse espaco fisicamente em vez de destruir e recriar.
    _jsonAtual.reserve(300);
    
    // Assim como a String, o vector tambem aloca memoria dinamicamente
    _erros.reserve(5);
}

void scTelemetriaBase::iniciarPacote() {
    _jsonAtual = "";
    _erros.clear();
    
    // Montagem ultraleve manual para evitar o peso do ArduinoJson a cada loop
    // {"mac_origem":"AA:BB:CC","tipo":"M1","dados":{
    _jsonAtual += "{\"mac_origem\":\"" + _macOrigem + "\",";
    _jsonAtual += "\"tipo\":\"" + _tipoModulo + "\",";
    _jsonAtual += "\"dados\":{";
    
    // Atributos Universais embutidos (Heartbeat de Saude)
    _jsonAtual += "\"status\":\"online\",";
    
    int rssi = WiFi.RSSI();
    _jsonAtual += "\"rssi\":" + String(rssi) + ",";
    
    if (_saude != nullptr) {
        uint32_t upt = _saude->getUptimeEstavelSegundos();
        _jsonAtual += "\"upt_est\":" + String(upt) + ",";
    }
}

void scTelemetriaBase::adicionarInteiro(const String& chave, int valor) {
    _jsonAtual += "\"" + chave + "\":" + String(valor) + ",";
}

void scTelemetriaBase::adicionarFloat(const String& chave, float valor, int casasDecimais) {
    _jsonAtual += "\"" + chave + "\":" + String(valor, casasDecimais) + ",";
}

void scTelemetriaBase::adicionarBool(const String& chave, bool valor) {
    _jsonAtual += "\"" + chave + "\":" + (valor ? String("true") : String("false")) + ",";
}

void scTelemetriaBase::adicionarString(const String& chave, const String& valor) {
    _jsonAtual += "\"" + chave + "\":\"" + valor + "\",";
}

void scTelemetriaBase::adicionarErro(const String& erroCodigo) {
    _erros.push_back(erroCodigo);
}

void scTelemetriaBase::despachar() {
    // Remove a ultima virgula pendente dos dados, caso exista
    if (_jsonAtual.endsWith(",")) {
        _jsonAtual.remove(_jsonAtual.length() - 1);
    }
    
    // Anexa os erros pendentes
    _jsonAtual += ",\"err\":[";
    for (size_t i = 0; i < _erros.size(); i++) {
        _jsonAtual += "\"" + _erros[i] + "\"";
        if (i < _erros.size() - 1) _jsonAtual += ",";
    }
    _jsonAtual += "]}"; // Fecha a array err e o objeto dados
    _jsonAtual += "}";  // Fecha a raiz do JSON
    
    // AUDITORIA CRITICA: O Limite Fisico do ESP-NOW
    // O payload nao pode ultrapassar 250 bytes sob pena de falha silenciosa na camada rádio.
    int tamanho = _jsonAtual.length();
    
    if (tamanho > 250) {
        if (_logger) {
            _logger->erro("TELEMETRIA", "OVERFLOW CRITICO! Payload com " + String(tamanho) + " bytes (Max: 250).");
            _logger->erro("TELEMETRIA", "Trecho: " + _jsonAtual.substring(0, 100) + "...");
        }
        
        // Em vez de morrer calado, envia um pacote de SOS emergencial para o Backend
        String sos = "{\"mac_origem\":\"" + _macOrigem + "\",\"tipo\":\"" + _tipoModulo + "\",\"dados\":{\"status\":\"online\",\"err\":[\"OVERFLOW_250_BYTES\"]}}";
        if (_mqtt != nullptr) _mqtt->enviarJSON(sos);
        
        // Descarta o pacote obeso para proteger a scTransceptorESPNow de corrupcao de pilha
        return; 
    }
    
    // Envia o JSON higienizado e dentro do limite
    if (_mqtt != nullptr) {
        _mqtt->enviarJSON(_jsonAtual);
    }
}
