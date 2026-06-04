#include "scLogger.h"
#include "scRelogioSincronizado.h"
#include "scArmazenamentoLocal.h"
#include "scMQTTLib.h"

scLogger::scLogger() {
    _outputSerial = true;
    _relogio = nullptr;
    _armazenamento = nullptr;
    _mqtt = nullptr;
}

void scLogger::inicializar(bool outputSerial, scRelogioSincronizado* relogio, scArmazenamentoLocal* armazenamento, scMQTTLib* mqtt) {
    _outputSerial = outputSerial;
    _relogio = relogio;
    _armazenamento = armazenamento;
    _mqtt = mqtt;
}

void scLogger::registrar(NivelLog nivel, String origem, String mensagem) {
    String msgFormatada = formatarMensagem(nivel, origem, mensagem);
    
    // 1. Output de Desenvolvimento
    if (_outputSerial) {
        Serial.println(msgFormatada);
    }
    
    // 2. Output de Producao (Somente erros)
    if (nivel == LOG_ERROR) {
        bool enviado = false;
        
        // Tenta mandar para o Backend imediatamente via MQTT
        if (_mqtt != nullptr && _mqtt->internetDisponivel()) {
            enviado = _mqtt->enviar("crash_" + origem, mensagem);
        }
        
        // Se falhou (sem internet ou MQTT fora) e temos o LittleFS, guarda fisicamente
        if (!enviado && _armazenamento != nullptr) {
            String chave = "crash_" + origem;
            _armazenamento->salvarChaveValor(chave, msgFormatada);
            _armazenamento->commitarAlteracoes();
        }
    }
}

void scLogger::despacharErrosPendentes() {
    if (_mqtt == nullptr || !_mqtt->internetDisponivel() || _armazenamento == nullptr) {
        return; // Nao tem condicoes de despachar agora
    }

    // Pega todas as chaves salvas que comecam com "crash_" (separadas por virgula)
    String chavesPulo = _armazenamento->listarChavesComPrefixo("crash_");
    if (chavesPulo.length() == 0) return;

    // Divisao rudimentar da string separada por virgula (simulacao)
    int startIndex = 0;
    while (startIndex < chavesPulo.length()) {
        int indexVirgula = chavesPulo.indexOf(',', startIndex);
        if (indexVirgula == -1) indexVirgula = chavesPulo.length();
        
        String chave = chavesPulo.substring(startIndex, indexVirgula);
        
        if (chave.length() > 0) {
            String mensagemAntiga = _armazenamento->obterValor(chave);
            if (mensagemAntiga.length() > 0) {
                // Tenta enviar o log morto-vivo pro MQTT
                bool enviado = _mqtt->enviar(chave, mensagemAntiga);
                if (enviado) {
                    // Se foi pro ar com sucesso, apaga fisicamente para nao criar loop
                    _armazenamento->removerChave(chave);
                    if (_outputSerial) {
                        Serial.println("[LOGGER] Erro pendente despachado: " + chave);
                    }
                }
            }
        }
        startIndex = indexVirgula + 1;
    }
    
    _armazenamento->commitarAlteracoes();
}

void scLogger::info(String origem, String mensagem) { 
    registrar(LOG_INFO, origem, mensagem); 
}

void scLogger::warn(String origem, String mensagem) { 
    registrar(LOG_WARNING, origem, mensagem); 
}

void scLogger::erro(String origem, String mensagem) { 
    registrar(LOG_ERROR, origem, mensagem); 
}

String scLogger::formatarMensagem(NivelLog nivel, String origem, String mensagem) {
    String prefixo = "";
    if (nivel == LOG_INFO) prefixo = "[INFO]";
    else if (nivel == LOG_WARNING) prefixo = "[WARN]";
    else if (nivel == LOG_ERROR) prefixo = "[ERR]";

    String timestamp = "";
    if (_relogio != nullptr) {
        // Traz o Unix Time garantido da rede ou do ESP-NOW
        timestamp = "[" + String(_relogio->obterHoraUnix()) + "] ";
    }

    return timestamp + prefixo + " [" + origem + "] " + mensagem;
}
