#ifndef SC_GESTOR_DISPOSITIVOS_M3_H
#define SC_GESTOR_DISPOSITIVOS_M3_H

#include <Arduino.h>
#include <ArduinoJson.h> // Para StaticJsonDocument
#include <SD.h>
#include <string.h>

class scLogger;

#define MAX_DISPOSITIVOS 20
#define TAM_MAC 18
#define TAM_NOME 32

struct DispositivoAlias {
    char mac[TAM_MAC];
    char apelido[TAM_NOME];
    bool ocupado;
};

class scGestorDispositivosM3 {
private:
    scLogger* _logger;
    DispositivoAlias _dicionario[MAX_DISPOSITIVOS];
    const char* _caminhoArquivo = "/apelidos.json";

    // Busca o indice de um MAC cadastrado, retorna -1 se nao achar
    int _encontrarIndice(const char* mac);
    
    // Procura o primeiro slot vazio na memoria (ocupado == false)
    int _encontrarSlotVazio();

public:
    scGestorDispositivosM3();

    void inicializar(scLogger* logger);

    // Le o arquivo /apelidos.json do SD e preenche a RAM
    void carregarDoSD();

    // Salva a RAM de volta no arquivo do SD
    bool salvarNoSD();

    // Adiciona ou atualiza o nome de um dispositivo na RAM
    bool adicionarApelido(const char* mac, const char* apelido);

    // Recupera o nome de um MAC. Se nao achar, copia o proprio MAC mascarado no buffer.
    void obterApelido(const char* mac, char* bufferSaida, size_t maxLen);
};

#endif // SC_GESTOR_DISPOSITIVOS_M3_H
