#include "scAgendadorTarefasM3.h"

// Ponteiro global C-Like para permitir que callbacks puros invoquem métodos de tela
static scGestorTelasM3* ptrTelasGlobal = nullptr;

static void _cbSlideshowInterrompido() {
    if (ptrTelasGlobal) {
        ptrTelasGlobal->mostrarDashboardCompleto();
    }
}

#include <WiFi.h>
// O MAC default para caso falhe na detecção via WiFi
#define MAC_M3 "M3:00:11:22:33:44"

scAgendadorTarefasM3::scAgendadorTarefasM3() : 
    _buzzer(PINO_BUZZER),
    _mqtt("", "") { // Inicia vazio. Será preenchido na inicialização via EEPROM
}

void scAgendadorTarefasM3::inicializar() {
    // 1. Logs Iniciais
    _logger.inicializar();
    _logger.info("M3", "=== INICIANDO SISTEMA M3 BARE-METAL ===");

    // 1.5. Infraestrutura de Nuvem / Armazenamento Global
    _eeprom.inicializar(); // Inicializa NVS (Non-Volatile Storage) do ESP32
    
    String wifiSSID = _eeprom.obterValor("WIFI_SSID");
    String wifiPass = _eeprom.obterValor("WIFI_PASS");
    String mqttUser = _eeprom.obterValor("MQTT_USER");
    String mqttPass = _eeprom.obterValor("MQTT_PASS");
    
    _mqtt.setCredenciais(mqttUser.c_str(), mqttPass.c_str());
    _mqtt.iniciar();
    
    _rede.configurarComoHub(true); // Impede que o Hub pule de canal ESP-NOW
    _rede.injetarMQTT(&_mqtt);     // Para detecção avançada de Roteador Zumbi
    _rede.inicializar(wifiSSID.c_str(), wifiPass.c_str(), &_logger); // Wi-Fi em Background
    _ota.inicializar(&_eeprom, &_logger, &_mqtt, "Módulo 3 (Hub Principal)");

    // 2. Saúde e Hardware Básico
    _rtc.inicializar(&_logger);
    _buzzer.inicializar(&_logger);
    _saude.inicializar(&_logger, &_rtc, &_sdCard, &_buzzer);

    // 3. Sistema de Arquivos (Base para Gráficos e Dicionários)
    // No S2, o Arduino Core as vezes mapeia o FSPI/SPI padrão para pinos errados. 
    // É vital fixar a trifásica de pinos (SCK=36, MISO=37, MOSI=35) ANTES do SD.begin.
    
    // BARE-METAL SPI FIX: Prevenir colisão no barramento desativando todos os escravos!
    pinMode(PINO_CS_SD, OUTPUT); digitalWrite(PINO_CS_SD, HIGH);
    pinMode(34, OUTPUT);         digitalWrite(34, HIGH); // TFT CS (PIN.txt)
    pinMode(12, OUTPUT);         digitalWrite(12, HIGH); // Touch CS (PIN.txt)

    SPI.begin(36, 37, 35, -1); 
    _sdCard.inicializar(PINO_CS_SD, &_logger);
    _gestorDispositivos.inicializar(&_logger);
    _gestorDispositivos.carregarDoSD(); // Puxa os apelidos da memória não volátil (EEPROM/SD)

    // 4. Lógica de Malha (Mesh) e Radar
    _radar.inicializar(&_logger, &_buzzer);
    
    // O Roteador Bridge depende do SD para log de lote e do Radar para avisar pings vitais
    _roteador.inicializar(&_logger, &_sdCard, &_radar);
    
    // O Despachante depende do Hardware Colateral e do Dicionário
    _despachante.inicializar(WiFi.macAddress().c_str(), &_logger, &_buzzer, &_gestorDispositivos);

    // 5. Placa de Video (Aloca a RAM Estática/Dinâmica)
    if (_motorGrafico.inicializar(&_logger, &_saude)) {
        // O Slideshow precisa do LVGL de pé para mapear a pasta /Imagens
        _slideshow.inicializar(&_logger);

        // Injeta os componentes base no Gestor de Telas para que a UI consiga interagir com o Sistema
        _gestorTelas.inicializar(&_logger, &_radar, &_gestorDispositivos, &_eeprom, &_mqtt, &_rtc, &_rede, &_roteador);
        ptrTelasGlobal = &_gestorTelas;
    } else {
        _logger.erro("M3", "Telas graficas abortadas devido a falha critica no Motor Grafico.");
    }

    // 6. Telemetria (Observador Passivo)
    _telemetria.inicializar(WiFi.macAddress().c_str(), &_logger, &_rtc, &_saude, &_sdCard, &_mqtt);

    _logger.info("scAgendadorTarefasM3", "=== BOOT CONCLUIDO COM SUCESSO. INGRESSANDO NO SUPER LOOP. ===");
}

void scAgendadorTarefasM3::processar() {
    uint32_t agora = millis();

    // 1. Manutenção Contínua da Conexão com Nuvem (Non-Blocking)
    _rede.atualizar();
    _mqtt.manterConexao();
    _ota.tratarLocalOTA();

    // 2. Proteção de Hardware e Engine LVGL (Relógio do Bare-Metal)
    // Sem isso, as animações congelam e o timeout de toque não funciona!
    static uint32_t ultimoTick = 0;
    lv_tick_inc(agora - ultimoTick);
    ultimoTick = agora;

    // 2. Drena a Fila do Rádio (Evita colapso do ESP-NOW)
    _roteador.processar();

    // 3. Verifica mortes na malha (Dead Man's Switch)
    _radar.processar();

    // 4. Lógica Inteligente do Slideshow
    if (!_slideshow.isAtivo()) {
        // Se a tela ficou 60 segundos sem ninguem tocar, inicia as fotos!
        if (lv_disp_get_inactive_time(lv_disp_get_default()) > 60000) {
            _slideshow.iniciar(lv_layer_top(), _cbSlideshowInterrompido);
        }
    }

    // 5. Motor Visual
    _motorGrafico.processar();
    _gestorTelas.atualizarMenuInteligente(); // Cores dinâmicas nos Dashboards
    _gestorTelas.processar();

    // Anúncio periódico do Hub na Malha (a cada 10s) para resgatar nós offline/perdidos
    static uint32_t ultimoAnuncio = 0;
    if (millis() - ultimoAnuncio > 10000) {
        ultimoAnuncio = millis();
        String meuMac = WiFi.macAddress();
        
        StaticJsonDocument<256> doc;
        doc["mac_origem"] = "HUB";
        doc["mac_destino"] = "ALL";
        doc["cmd"] = "set_peer_mac";
        
        JsonObject args = doc.createNestedObject("args");
        args["tipo_alvo"] = "HUB";
        args["mac_alvo"] = meuMac;

        char buffer[256];
        serializeJson(doc, buffer);
        
        // Envia o grito pelo rádio para quem estiver sem internet escutando (ou para pings novos)
        _roteador.enviarBroadcast(buffer);
        _logger.info("scAgendador", "Identidade do Hub (set_peer_mac) propagada via ESP-NOW.");
    }
    _slideshow.processar(); // Troca as fotos a cada 10s se estiver ativo
    _buzzer.atualizar(); // Permite que a sirene não-bloqueante pulse

    // 6. Temporizador da Telemetria (A cada 5 minutos tira uma "foto" da saude)
    static uint32_t ultimoLogTelemetria = 0;
    if (agora - ultimoLogTelemetria > 300000) { 
        _telemetria.registrarCena(); // Envia JSON local pro SD
        ultimoLogTelemetria = agora;
    }

    // 7. Cão de Guarda de Milissegundos
    _saude.processar();

    // 8. Drenagem da Fila Idempotente (Se houver internet e SD Card)
    if (_mqtt.internetDisponivel() && _sdCard.isAtivo()) {
        File fila = _sdCard.abrirFilaOffline();
        if (fila) {
            bool falhouAlgum = false;
            while (fila.available()) {
                String linha = fila.readStringUntil('\n');
                linha.trim();
                if (linha.length() > 0) {
                    if (!_mqtt.enviarJSON(linha.c_str())) {
                        falhouAlgum = true;
                        break; // Se caiu, para e tenta de novo no proximo loop
                    }
                }
            }
            fila.close();
            
            // Se varreu tudo com sucesso pro broker, limpa o arquivo!
            if (!falhouAlgum) {
                _sdCard.destruirFilaOffline();
            }
        }
    }
}
