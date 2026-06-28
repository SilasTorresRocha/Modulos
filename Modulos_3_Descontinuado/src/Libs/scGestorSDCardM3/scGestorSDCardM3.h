#ifndef SC_GESTOR_SD_CARD_M3_H
#define SC_GESTOR_SD_CARD_M3_H

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "../../LibsGlobais/scLogger/scLogger.h"

class scGestorSDCardM3 {
public:
    scGestorSDCardM3();

    // Inicia a comunicação SPI com o cartão SD
    void inicializar(scLogger* logger, uint8_t pinoCS, SPIClass* spi = nullptr);

    // Salva uma linha de log/telemetria no final do arquivo de fallback
    bool gravarTelemetriaOffline(const String& payloadJSON);

    // Varredura para descarregar o banco (Lê linha a linha sem travar a RAM)
    // Retorna vazio ("") quando o arquivo chegar ao fim ou não existir.
    String extrairProximaLinhaOffline();
    
    // Confirma que a linha anterior foi despachada com sucesso e avança o cursor (ou deleta o arquivo se chegou no fim)
    void confirmarEnvioLinha();
    
    // Deleta o banco de dados temporário de offline para não re-enviar dados antigos
    void limparBancoOffline();

    // Diagnostico para o scMonitorSaudeM3
    bool FalhaComunicacao() const;

private:
    scLogger* _logger;
    uint8_t _pinoCS;
    bool _sdConectado;
    
    // Controle do Leitor (Cursor) para enviar aos poucos sem sobrecarregar a rede
    File _arquivoLeitura;
    uint32_t _posicaoLeitura;
    bool _leituraEmAndamento;
    
    const char* CAMINHO_DB_OFFLINE = "/offline.jsonl";
};

#endif // SC_GESTOR_SD_CARD_M3_H
