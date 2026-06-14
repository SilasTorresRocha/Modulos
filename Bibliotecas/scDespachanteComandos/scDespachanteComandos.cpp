#include "scDespachanteComandos.h"
#include "../scRelogioSincronizado/scRelogioSincronizado.h"
#include "../scConfigOTA/scConfigOTA.h"
#include "../scLogger/scLogger.h"
#include "../scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "../scTransceptorESPNow/scTransceptorESPNow.h"

scDespachanteComandos::scDespachanteComandos() {
    _macLocal = "";
    _relogio = nullptr;
    _ota = nullptr;
    _logger = nullptr;
    _armazenamento = nullptr;
    _transceptor = nullptr;
    _callbackLocal = nullptr;
}

void scDespachanteComandos::inicializar(String macDestaPlaca, scRelogioSincronizado* relogio, scConfigOTA* ota, scLogger* logger, scArmazenamentoLocal* armazenamento, scTransceptorESPNow* transceptor) {
    _macLocal = macDestaPlaca;
    _relogio = relogio;
    _ota = ota;
    _logger = logger;
    _armazenamento = armazenamento;
    _transceptor = transceptor;
}

void scDespachanteComandos::registrarCallbackLocal(CallbackComandoLocal callback) {
    _callbackLocal = callback;
}

void scDespachanteComandos::processarPayload(const char* macOrigemTransceptor, const char* payload) {
    // PROTECAO DE MEMORIA (RAII): 
    // O JsonDocument existe apenas durante esta execucao. A RAM sera devolvida ao final.
    JsonDocument doc; 
    
    DeserializationError erro = deserializeJson(doc, payload);

    if (erro) {
        if (_logger) _logger->erro("DESPACHANTE", "Pacote JSON invalido/corrompido: " + String(erro.c_str()));
        return; // Aborta e protege a placa de travamentos
    }

    // 1. Validacao de Destino
    const char* macDestino = doc["mac_destino"];
    if (!macDestino) return; // Ignora se nao tiver o campo

    String destinoStr = String(macDestino);
    
    // Se a mensagem nao for para mim ("mac exato") e nao for para todos ("ALL"), ignoro.
    if (destinoStr != _macLocal && destinoStr != "ALL") {
        return; 
    }

    // 2. Extracao do Comando
    const char* cmd = doc["cmd"];
    if (!cmd) return;
    
    // Validacao de Timestamp (Anti-Replay Attack)
    if (doc.containsKey("ts")) {
        if (_relogio && _relogio->estaSincronizado()) {
            uint32_t ts_pacote = doc["ts"].as<uint32_t>();
            uint32_t ts_agora = _relogio->obterHoraUnix();
            if (ts_agora > ts_pacote + 5 || ts_pacote > ts_agora + 5) {
                if (_logger) _logger->erro("DESPACHANTE", "Pacote descartado: Replay Attack (TS vencido ou futuro).");
                return;
            }
        } else {
            if (_logger) _logger->warn("DESPACHANTE", "Pacote com TS recebido, mas relogio local nao esta sincronizado. Processando mesmo assim.");
        }
    }
    
    String comandoStr = String(cmd);
    JsonVariant args = doc["args"];

    // -------------------------------------------------------------
    // BLOCO DE RESOLUCAO UNIVERSAL (Infraestrutura)
    // Tudo o que e comum aos Modulos 1, 2, 3 e 4 morre aqui.
    // -------------------------------------------------------------
    
    if (comandoStr == "sincronizar_relogio") {
        if (_relogio && args["timestamp_unix"]) {
            _relogio->definirHoraManualmente(args["timestamp_unix"].as<uint32_t>());
            if (_logger) _logger->info("DESPACHANTE", "Hora sincronizada remotamente.");
        }
        return; // Comando resolvido
    }
    
    if (comandoStr == "atualizar_firmware") {
        if (_ota && args["url_binario"]) {
            _ota->agendarNovaURL(args["url_binario"].as<String>());
            if (_logger) _logger->info("DESPACHANTE", "Nova URL de firmware agendada.");
        }
        return;
    }
    
    if (comandoStr == "reiniciar_dispositivo") {
        if (_logger) _logger->info("DESPACHANTE", "Reboot forcado pelo Hub/Nuvem.");
        delay(500); 
        ESP.restart();
        return;
    }
    
    if (comandoStr == "update_wifi") {
        if (_armazenamento && args["ssid"] && args["pass"]) {
            _armazenamento->salvarChaveValor("wifi_ssid", args["ssid"].as<String>());
            _armazenamento->salvarChaveValor("wifi_pass", args["pass"].as<String>());
            _armazenamento->commitarAlteracoes();
            if (_logger) _logger->info("DESPACHANTE", "Credenciais de Wi-Fi atualizadas. Reiniciando...");
            delay(500);
            ESP.restart();
        }
        return;
    }
    
    if (comandoStr == "update_libmqtt") {
        if (_armazenamento && args["usuario"] && args["senha"]) {
            _armazenamento->salvarChaveValor("mqtt_usr", args["usuario"].as<String>());
            _armazenamento->salvarChaveValor("mqtt_pass", args["senha"].as<String>());
            _armazenamento->commitarAlteracoes();
            if (_logger) _logger->info("DESPACHANTE", "Credenciais MQTT atualizadas. Reiniciando...");
            delay(500);
            ESP.restart();
        }
        return;
    }

    if (comandoStr == "update_espnow_key") {
        if (_armazenamento && _transceptor && args["chave_simetrica"]) {
            String novaChave = args["chave_simetrica"].as<String>();
            _armazenamento->salvarChaveValor("espnow_key", novaChave);
            _armazenamento->commitarAlteracoes();
            _transceptor->configurarCriptografia(novaChave, novaChave);
            if (_logger) _logger->info("DESPACHANTE", "Nova chave ESP-NOW salva e ativada.");
        }
        return;
    }
    
    if (comandoStr == "set_peer_mac") {
        if (_logger) _logger->info("DESPACHANTE", "Recebido provisionamento P2P set_peer_mac.");
        
        if (_armazenamento && args["tipo_alvo"] && args["mac_alvo"]) {
            String chave = "peer_" + args["tipo_alvo"].as<String>();
            String mac = args["mac_alvo"].as<String>();
            _armazenamento->salvarChaveValor(chave, mac);
            _armazenamento->commitarAlteracoes();
            if (_logger) _logger->info("DESPACHANTE", "P2P MAC salvo na flash: " + chave);
        }
        
        // Repassa o comando para o arquivo principal (.ino) carregar na RAM imediatamente
        if (_callbackLocal != nullptr) {
            _callbackLocal(cmd, args);
        }
        return;
    }
    
    if (comandoStr == "solicitar_status") {
        if (_logger) _logger->info("DESPACHANTE", "Solicitacao de Heartbeat instantaneo recebida.");
        if (_callbackLocal != nullptr) {
            _callbackLocal(cmd, args);
        }
        return;
    }

    // -------------------------------------------------------------
    // BLOCO DE REPASSE (Regra de Negocio do Modulo)
    // -------------------------------------------------------------
    
    // Se o comando nao for universal (ex: "set_rele", "configurar_operacao"),
    // passamos a bola para o arquivo .ino resolver.
    if (_callbackLocal != nullptr) {
        _callbackLocal(cmd, args);
    }
}
