#include "scGestorSDCardM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

scGestorSDCardM3::scGestorSDCardM3() : _logger(nullptr), _pinoCS(0), _ativo(false) {
}

bool scGestorSDCardM3::inicializar(uint8_t pinoCS, scLogger* logger, SPIClass* spi) {
    _logger = logger;
    _pinoCS = pinoCS;

    // Tenta montar o SD Card. Usando frequencia estavel de 4MHz para compatibilidade SPI garantida
    if (!SD.begin(_pinoCS, *spi, 4000000)) {
        if (_logger != nullptr) {
            _logger->erro("scGestorSDCardM3", "Falha critica: SD Card nao encontrado no barramento SPI!");
        }
        _ativo = false;
        return false;
    }

    _ativo = true;
    if (_logger != nullptr) {
        _logger->info("scGestorSDCardM3", "SD Card montado com sucesso (SPI).");
    }

    // Estruturacao inicial do sistema de arquivos para UI e logs
    _verificarEstruturaPastas();

    return true;
}

void scGestorSDCardM3::_verificarEstruturaPastas() {
    if (!_ativo) return;

    if (!SD.exists(_pastaImagens)) {
        if (SD.mkdir(_pastaImagens)) {
            if (_logger != nullptr) {
                _logger->info("scGestorSDCardM3", "Pasta /Imagens criada na raiz do SD Card.");
            }
            
            // Cria o tutorial instruindo sobre performance Bare-Metal
            File arquivoTexto = SD.open("/Imagens/LEIA-ME.txt", FILE_WRITE);
            if (arquivoTexto) {
                arquivoTexto.println("=== MOTOR GRAFICO M3 (LVGL) ===");
                arquivoTexto.println("Coloque aqui suas imagens convertidas para .bin atraves do LVGL Image Converter (Color Format: True color).");
                arquivoTexto.println("AVISO: Arquivos .jpg e .png NAO funcionarao para poupar o processador.");
                arquivoTexto.close();
            }
        } else {
            if (_logger != nullptr) {
                _logger->erro("scGestorSDCardM3", "Falha ao tentar criar a pasta /Imagens.");
            }
        }
    }
}

bool scGestorSDCardM3::gravarLog(const char* mensagemErro) {
    if (!_ativo) return false;

    // Modo FILE_APPEND: Garante operacao o(1) instantanea, sem ler bytes anteriores

    File arquivo = SD.open(_caminhoCrashLog, FILE_APPEND);
    if (!arquivo) {
        if (_logger != nullptr) {
            _logger->erro("scGestorSDCardM3", "Falha ao abrir arquivo de crash logs.");
        }
        return false;
    }

    arquivo.println(mensagemErro);
    arquivo.close();
    
    return true;
}

bool scGestorSDCardM3::enfileirarOffline(const char* payloadJson) {
    if (!_ativo) return false;

    // Anexa o pacote cru no lote para posterior despache
    File arquivo = SD.open(_caminhoFilaOffline, FILE_APPEND);
    if (!arquivo) {
        if (_logger != nullptr) {
            _logger->erro("scGestorSDCardM3", "Nao foi possivel abrir a fila_offline.jsonl para append.");
        }
        return false;
    }

    arquivo.println(payloadJson);
    arquivo.close();
    
    return true;
}

File scGestorSDCardM3::abrirFilaOffline() {
    // Retorna o ponteiro para o RoteadorBridge consumir.
    // O chamador usa arquivo.readStringUntil('\n') e arquivo.close()
    if (!_ativo) return File();
    return SD.open(_caminhoFilaOffline, FILE_READ);
}

void scGestorSDCardM3::destruirFilaOffline() {
    if (!_ativo) return;

    if (SD.exists(_caminhoFilaOffline)) {
        SD.remove(_caminhoFilaOffline);
        if (_logger != nullptr) {
            _logger->info("scGestorSDCardM3", "Fila offline despachada para a nuvem e limpa do SD Card (Idempotencia).");
        }
    }
}

bool scGestorSDCardM3::isAtivo() const {
    return _ativo;
}
