#ifndef SC_TRANSCEPTOR_ESPNOW_H
#define SC_TRANSCEPTOR_ESPNOW_H

#include <Arduino.h>

#if defined(ESP8266)
    #include <espnow.h>
    #include <ESP8266WiFi.h>
#elif defined(ESP32)
    #include <esp_now.h>
    #include <WiFi.h>
#endif

class scLogger;

class scTransceptorESPNow {
public:
    scTransceptorESPNow();

    // Inicializa a camada de hardware do rádio e registra os callbacks nativos
    void inicializar(scLogger* logger);

    // Módulo 4: Injeta as chaves simétricas provindas do Backend (A "Senha da Casa")
    // Se a placa for ESP8266, esse método apenas gerará um log de aviso de limitação de hardware.
    void configurarCriptografia(String pmk, String lmk);

    // Envia o payload respeitando o limite físico de 250 bytes.
    // O MAC Destino é dinamicamente cadastrado (P2P Discovery) caso não exista na lista de Peers.
    bool enviarPacote(uint8_t* macDestino, const String& payload);
    
    // Callback para entregar o pacote recebido à scDespachanteComandos
    typedef void (*AcaoRecebimento)(const char* macOrigem, const char* payload);
    void definirRecebimento(AcaoRecebimento acao);

private:
    scLogger* _logger;
    AcaoRecebimento _acaoRecebimento;
    
    // Controle da Criptografia
    bool _usarCriptografia;
    uint8_t _lmk[16];
    uint8_t _pmk[16];

    // O Roteador Estático: Como as APIs da Espressif exigem callbacks em C puro,
    // usamos esse ponteiro global para devolver a execução ao objeto C++.
    static scTransceptorESPNow* _instanciaGlobal;

    // Gerenciador Dinâmico de Peers (Para M1 falar com M2 sem setup prévio)
    void adicionarPeerDinamico(uint8_t* mac);

    // Helper para formatar o MAC em um buffer (Zero Alocação no Heap)
    void macParaBuffer(const uint8_t* macBytes, char* buffer);

    // Callbacks Nativos da Espressif (Assinaturas variam por Arquitetura)
#if defined(ESP8266)
    static void callbackRecebimento(uint8_t * mac, uint8_t *dadosRecebidos, uint8_t tamanho);
    static void callbackEnvio(uint8_t *macDestino, uint8_t statusEnvio);
#elif defined(ESP32)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    static void callbackRecebimento(const esp_now_recv_info_t *info, const uint8_t *dadosRecebidos, int tamanho);
    static void callbackEnvio(const esp_now_send_info_t *info, esp_now_send_status_t statusEnvio);
#else
    static void callbackRecebimento(const uint8_t *mac_addr, const uint8_t *dadosRecebidos, int tamanho);
    static void callbackEnvio(const uint8_t *macDestino, esp_now_send_status_t statusEnvio);
#endif
#endif
};

#endif // SC_TRANSCEPTOR_ESPNOW_H
