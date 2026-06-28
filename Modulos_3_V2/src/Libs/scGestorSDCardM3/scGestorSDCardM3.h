#ifndef SC_GESTOR_SDCARD_M3_H
#define SC_GESTOR_SDCARD_M3_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

class scLogger;

#define PINO_CS_SD 10
#define MAX_ARQUIVOS_FILA_OFFLINE 50

class scGestorSDCardM3 {
private:
    scLogger* _logger;
    uint8_t _pinoCS;
    bool _ativo;
    
    // Nomes dos arquivos de servico
    const char* _caminhoFilaOffline = "/fila_offline.jsonl";
    const char* _caminhoCrashLog = "/crash_log.jsonl";
    const char* _pastaImagens = "/Imagens";

    // Metodo privado para estruturar a raiz do SD
    void _verificarEstruturaPastas();

public:
    scGestorSDCardM3();

    // Inicializa o barramento SPI do SD Card. 
    // Pode receber o ponteiro de SPI se for compartilhado (vital para dividir o SPI com o Display)
    bool inicializar(uint8_t pinoCS, scLogger* logger, SPIClass* spi = &SPI);

    // Salva logs pesados no SD para nao estourar a RAM
    bool gravarLog(const char* mensagemErro);

    // ==========================================
    // Fila Offline (Lote Idempotente)
    // ==========================================
    
    // Adiciona o pacote cru no final do arquivo de lote (Zero Heap String)
    bool enfileirarOffline(const char* payloadJson);

    // Retorna o objeto File apontado para leitura da fila offline
    File abrirFilaOffline();

    // Apaga fisicamente o arquivo de fila apos transmissao completa para nuvem
    void destruirFilaOffline();

    // Getter basico
    bool isAtivo() const;
};

#endif // SC_GESTOR_SDCARD_M3_H
