#include "scGestorSDCardM3.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 15
#endif

scGestorSDCardM3::scGestorSDCardM3() {
    _logger = nullptr;
    _sdConectado = false;
    _posicaoLeitura = 0;
    _leituraEmAndamento = false;
}

void scGestorSDCardM3::inicializar(scLogger* logger, uint8_t pinoCS, SPIClass* spi) {
    _logger = logger;
    _pinoCS = pinoCS;

    if (!_logger) {
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) { digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); delay(100); yield(); }
    }

    _logger->info("SD_M3", "Montando SD Card no pino CS " + String(_pinoCS) + "...");

    // Tenta montar o sistema de arquivos
    bool sucesso = (spi != nullptr) ? SD.begin(_pinoCS, *spi) : SD.begin(_pinoCS);
    
    if (!sucesso) {
        _sdConectado = false;
        _logger->erro("SD_M3", "FALHA: Modulo SD Card nao encontrado ou cartao ausente/corrompido!");
    } else {
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            _sdConectado = false;
            _logger->erro("SD_M3", "Nenhum cartao inserido no modulo SD.");
            return;
        }
        
        _sdConectado = true;
        _logger->info("SD_M3", "SD Card montado com sucesso! Capacidade: " + String(SD.cardSize() / (1024 * 1024)) + "MB");
    }
}

bool scGestorSDCardM3::gravarTelemetriaOffline(const String& payloadJSON) {
    if (!_sdConectado) return false;
    
    // Impede gravação simultânea com leitura (se estivéssemos lendo e a rede cair de novo)
    if (_leituraEmAndamento) {
        _arquivoLeitura.close();
        _leituraEmAndamento = false;
    }

    File db = SD.open(CAMINHO_DB_OFFLINE, FILE_APPEND);
    if (!db) {
        _logger->erro("SD_M3", "Erro ao abrir JSONL para gravar (APPEND).");
        return false;
    }
    
    db.println(payloadJSON); // Grava o payload e adiciona o '\n' automaticamente
    db.close();
    
    _logger->info("SD_M3", "Telemetria salva offline (Tamanho BD: " + String(db.size()) + " bytes)");
    return true;
}

String scGestorSDCardM3::extrairProximaLinhaOffline() {
    if (!_sdConectado) return "";
    
    // Se não existir o arquivo, não há o que enviar
    if (!SD.exists(CAMINHO_DB_OFFLINE)) return "";
    
    if (!_leituraEmAndamento) {
        _arquivoLeitura = SD.open(CAMINHO_DB_OFFLINE, FILE_READ);
        if (!_arquivoLeitura) {
            _logger->erro("SD_M3", "Erro ao abrir JSONL para leitura.");
            return "";
        }
        _leituraEmAndamento = true;
        _posicaoLeitura = 0;
        _logger->info("SD_M3", "Iniciando descarga de banco de dados offline...");
    }
    
    // Restaura o ponteiro pro lugar onde parou
    _arquivoLeitura.seek(_posicaoLeitura);
    
    if (_arquivoLeitura.available()) {
        String linha = _arquivoLeitura.readStringUntil('\n');
        linha.trim(); // Limpa \r\n residuais
        return linha;
    } else {
        // Fim do arquivo
        _arquivoLeitura.close();
        _leituraEmAndamento = false;
        limparBancoOffline(); // Deleta o arquivo pois todos foram despachados
        _logger->info("SD_M3", "Descarga offline concluida. Banco limpo.");
        return "";
    }
}

void scGestorSDCardM3::confirmarEnvioLinha() {
    if (_leituraEmAndamento && _arquivoLeitura) {
        // Apenas atualizamos a variavel de posicao para a proxima vez que chamar o extrairProximaLinhaOffline
        _posicaoLeitura = _arquivoLeitura.position();
    }
}

void scGestorSDCardM3::limparBancoOffline() {
    if (!_sdConectado) return;
    if (_leituraEmAndamento) {
        _arquivoLeitura.close();
        _leituraEmAndamento = false;
    }
    if (SD.exists(CAMINHO_DB_OFFLINE)) {
        SD.remove(CAMINHO_DB_OFFLINE);
    }
    _posicaoLeitura = 0;
}

bool scGestorSDCardM3::FalhaComunicacao() const {
    return !_sdConectado;
}
