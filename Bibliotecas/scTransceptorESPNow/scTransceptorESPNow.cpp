#include "scTransceptorESPNow.h"

scTransceptorESPNow::scTransceptorESPNow() {
    _acaoRecebimento = nullptr;
}

void scTransceptorESPNow::inicializar() {
    // Inicializar radio ESP-NOW, registrar placas dinamicas
}

bool scTransceptorESPNow::enviarPacote(uint8_t* macDestino, String payload) {
    // Rejeita IMEDIATAMENTE se o pacote for maior que 250 bytes.
    // O ecossistema garante que os contratos JSON sejam menores que o limite nativo.
    if (payload.length() > 250) {
        return false; 
    }
    // Logica de esp_now_send
    return true;
}

void scTransceptorESPNow::definirRecebimento(AcaoRecebimento acao) {
    _acaoRecebimento = acao;
}

void scTransceptorESPNow::recebimento(uint8_t * mac, uint8_t *dadosRecebidos, uint8_t tamanho) {
    // Converte os dados brutos direto para String JSON (pois sabemos que esta completo)
    // Se a montagem for sucesso, chama a acao registrada na classe
}

void scTransceptorESPNow::envio(uint8_t *macDestino, uint8_t statusEnvio) {
    // Logica de confirmacao de entrega da placa
}
