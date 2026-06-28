#include "scGestorDispositivosM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

scGestorDispositivosM3::scGestorDispositivosM3() : _logger(nullptr) {
    // Inicializa todos os slots como livres na inicializacao
    for (int i = 0; i < MAX_DISPOSITIVOS; i++) {
        _dicionario[i].ocupado = false;
        memset(_dicionario[i].mac, 0, TAM_MAC);
        memset(_dicionario[i].apelido, 0, TAM_NOME);
    }
}

void scGestorDispositivosM3::inicializar(scLogger* logger) {
    _logger = logger;
    
    if (_logger != nullptr) {
        _logger->info("scGestorDispositivos", "Tradutor de Nomes inicializado (1KB RAM fixa alocada).");
    }
    
    carregarDoSD();
}

int scGestorDispositivosM3::_encontrarIndice(const char* mac) {
    for (int i = 0; i < MAX_DISPOSITIVOS; i++) {
        if (_dicionario[i].ocupado && strncmp(_dicionario[i].mac, mac, TAM_MAC - 1) == 0) {
            return i;
        }
    }
    return -1; // Nao encontrado
}

int scGestorDispositivosM3::_encontrarSlotVazio() {
    for (int i = 0; i < MAX_DISPOSITIVOS; i++) {
        if (!_dicionario[i].ocupado) {
            return i;
        }
    }
    return -1; // Sem espaco
}

bool scGestorDispositivosM3::adicionarApelido(const char* mac, const char* apelido) {
    int idx = _encontrarIndice(mac);
    
    if (idx == -1) {
        // Modulo novo, procura espaco livre
        idx = _encontrarSlotVazio();
        if (idx == -1) {
            if (_logger != nullptr) {
                _logger->erro("scGestorDispositivos", "Falha critica: Limite de 20 dispositivos atingido. Nao eh possivel adicionar mais apelidos.");
            }
            return false;
        }
    }

    // Copia segura de strings em C evitando Buffer Overflow
    strncpy(_dicionario[idx].mac, mac, TAM_MAC - 1);
    _dicionario[idx].mac[TAM_MAC - 1] = '\0'; // Garante fechamento
    
    strncpy(_dicionario[idx].apelido, apelido, TAM_NOME - 1);
    _dicionario[idx].apelido[TAM_NOME - 1] = '\0';
    
    _dicionario[idx].ocupado = true;
    
    return true;
}

void scGestorDispositivosM3::obterApelido(const char* mac, char* bufferSaida, size_t maxLen) {
    int idx = _encontrarIndice(mac);
    
    if (idx != -1) {
        // Encontrou, copia o apelido amigavel para quem chamou
        strncpy(bufferSaida, _dicionario[idx].apelido, maxLen - 1);
    } else {
        // Nao encontrou no dicionario, devolve o proprio MAC como paliativo ("Node AA:BB...")
        snprintf(bufferSaida, maxLen, "Node %s", mac);
    }
    
    // Trava de seguranca da linguagem C
    bufferSaida[maxLen - 1] = '\0';
}

void scGestorDispositivosM3::carregarDoSD() {
    if (!SD.exists(_caminhoArquivo)) {
        if (_logger != nullptr) {
            _logger->warn("scGestorDispositivos", "Arquivo /apelidos.json ausente. O dicionario operara limpo na RAM.");
        }
        return;
    }

    File arquivo = SD.open(_caminhoArquivo, FILE_READ);
    if (!arquivo) {
        if (_logger != nullptr) {
            _logger->erro("scGestorDispositivos", "Falha ao abrir /apelidos.json para leitura.");
        }
        return;
    }

    // Memoria estatica na Stack (sem new/malloc) suficiente para parsear 20 nodes
    StaticJsonDocument<1024> doc; 
    DeserializationError erro = deserializeJson(doc, arquivo);
    arquivo.close();

    if (erro) {
        if (_logger != nullptr) {
            _logger->erro("scGestorDispositivos", "Falha no parse (deserialize) do arquivo JSON de Apelidos.");
        }
        return;
    }

    // O JSON esperado eh: {"11:22:33...": "Quarto", "CC:DD:EE...": "Jardim"}
    JsonObject obj = doc.as<JsonObject>();
    for (JsonPair kv : obj) {
        adicionarApelido(kv.key().c_str(), kv.value().as<const char*>());
    }
    
    if (_logger != nullptr) {
        _logger->info("scGestorDispositivos", "Dicionario de nomes amigaveis carregado do SD para a RAM com sucesso.");
    }
}

bool scGestorDispositivosM3::salvarNoSD() {
    File arquivo = SD.open(_caminhoArquivo, FILE_WRITE); // Reescreve por cima
    if (!arquivo) {
        if (_logger != nullptr) {
            _logger->erro("scGestorDispositivos", "Falha ao abrir /apelidos.json para gravação SPI.");
        }
        return false;
    }

    StaticJsonDocument<1024> doc;
    
    for (int i = 0; i < MAX_DISPOSITIVOS; i++) {
        if (_dicionario[i].ocupado) {
            doc[_dicionario[i].mac] = _dicionario[i].apelido; // Indexa chave-valor
        }
    }

    if (serializeJson(doc, arquivo) == 0) {
        if (_logger != nullptr) {
            _logger->erro("scGestorDispositivos", "Falha de serializacao SPI gravando apelidos.");
        }
        arquivo.close();
        return false;
    }
    
    arquivo.close();
    
    if (_logger != nullptr) {
        _logger->info("scGestorDispositivos", "Apelidos salvos permanentemente no SD Card.");
    }
    
    return true;
}
