#include "scDespachanteComandosM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scGestorBuzzerM3/scGestorBuzzerM3.h"
#include "../scGestorDispositivosM3/scGestorDispositivosM3.h"

scDespachanteComandosM3::scDespachanteComandosM3() : 
    _logger(nullptr), _buzzer(nullptr), _gestorDispositivos(nullptr), _meuMac("") {
}

void scDespachanteComandosM3::inicializar(const char* meuMac, scLogger* logger, scGestorBuzzerM3* buzzer, scGestorDispositivosM3* gestorDispositivos) {
    _meuMac = meuMac;
    _logger = logger;
    _buzzer = buzzer;
    _gestorDispositivos = gestorDispositivos;

    if (_logger != nullptr) {
        _logger->info("scDespachanteComandos", "Cerebro Executor Ativo. Motor semantico operando.");
    }
}

void scDespachanteComandosM3::processarPacote(const char* payloadJson) {
    // Alocação extremamente segura: 512 bytes fixos na Stack da Thread/Loop. Zero Lixo na Heap.
    StaticJsonDocument<512> doc;
    DeserializationError erro = deserializeJson(doc, payloadJson);

    if (erro) {
        // Se nao for um JSON valido, o despachante so ignora. Pode ser lixo de rede.
        return;
    }

    // 1. Filtro de Endereçamento (Roteamento Físico)
    const char* macDestino = doc["mac_destino"] | "";
    if (strcmp(macDestino, _meuMac) != 0 && strcmp(macDestino, "ALL") != 0) {
        // Endereçado para outro modulo (ex: M1 ou M2). 
        // O Roteador já cuidou de gravar na Fila SD. O Despachante não se importa.
        return;
    }

    // 2. Extração do Comando (Roteamento Lógico)
    const char* comando = doc["cmd"] | "";
    if (strlen(comando) == 0) return;

    if (_logger != nullptr) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Comando local interceptado: %s", comando);
        _logger->info("scDespachanteComandos", buf);
    }

    // ==========================================
    // DECODIFICADOR DE ACOES LOCAIS
    // ==========================================

    // CMD: SET_ALIAS (Define um Apelido Humano para um MAC)
    // Permite que o usuario rebatize os modulos (Ex: de 11:22:33 para "Quarto Casal")
    if (strcmp(comando, "SET_ALIAS") == 0) {
        const char* macAlvo = doc["mac"] | "";
        const char* novoNome = doc["nome"] | "";
        
        if (strlen(macAlvo) > 0 && strlen(novoNome) > 0 && _gestorDispositivos != nullptr) {
            if (_gestorDispositivos->adicionarApelido(macAlvo, novoNome)) {
                _gestorDispositivos->salvarNoSD(); // Salva estado persistente (Offline Queue intocada)
                if (_buzzer != nullptr) _buzzer->tocarSucesso();
            }
        }
    }
    // CMD: MUTE_UI (Muta ou Desmuta Bips Táteis Locais)
    else if (strcmp(comando, "MUTE_UI") == 0) {
        bool estado = doc["mutado"] | false;
        if (_buzzer != nullptr) {
            _buzzer->setMute(estado);
            _buzzer->tocarSucesso(); // Confirma acusticamente (mesmo se desmutou)
        }
    }
    // CMD: SOFT_RESET (Ordem externa para reiniciar)
    else if (strcmp(comando, "SOFT_RESET") == 0) {
        if (_logger != nullptr) _logger->warn("scDespachanteComandos", "REINICIO ORDENADO REMOTAMENTE! Rebooting...");
        delay(300); // Pausa minima pro logger esvaziar a serial
        ESP.restart();
    }
    // CMD: UPDATE_WIFI (Atualizacao Universal de Credenciais de Rede)
    else if (strcmp(comando, "UPDATE_WIFI") == 0) {
        const char* ssid = doc["ssid"] | "";
        // No futuro, isso ativara a classe responsavel por salvar o NVS e conectar.
        if (_logger != nullptr) {
            char buf[80];
            snprintf(buf, sizeof(buf), "Processando atualizacao de credenciais para SSID: %s", ssid);
            _logger->info("scDespachanteComandos", buf);
        }
        if (_buzzer != nullptr) _buzzer->tocarSucesso();
    }
    else {
        if (_logger != nullptr) _logger->warn("scDespachanteComandos", "Comando recebido, porem desconhecido/nao implementado.");
    }
}
