#ifndef SC_TRANSCEPTOR_ESPNOW_H
#define SC_TRANSCEPTOR_ESPNOW_H

#include <Arduino.h>

class scTransceptorESPNow {
public:
    scTransceptorESPNow();
    void inicializar();
    bool enviarPacote(uint8_t* macDestino, String payload);
    
    // Funcao para registrar callback na camada superior (ex: Despachante)
    typedef void (*AcaoRecebimento)(String payload);
    void definirRecebimento(AcaoRecebimento acao);

private:
    AcaoRecebimento _acaoRecebimento;
    // Funcoes de resposta nativas do ESP-NOW
    static void recebimento(uint8_t * mac, uint8_t *dadosRecebidos, uint8_t tamanho);
    static void envio(uint8_t *macDestino, uint8_t statusEnvio);
};

#endif // SC_TRANSCEPTOR_ESPNOW_H
