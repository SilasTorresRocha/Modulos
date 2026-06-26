#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <U8g2lib.h>

// ==========================================
// BIBLIOTECAS GLOBAIS (Ecossistema)
// ==========================================
#include "src/LibsGlobais/scLogger/scLogger.h"
#include "src/LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "src/LibsGlobais/scAvisosSonoros/scAvisosSonoros.h"
#include "src/LibsGlobais/scGestorDisplay/scGestorDisplay.h"
#include "src/LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "src/LibsGlobais/scConfigOTA/scConfigOTA.h"
#include "src/LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "src/LibsGlobais/scTransceptorESPNow/scTransceptorESPNow.h"
#include "src/LibsGlobais/scGestorRede/scGestorRede.h"
#include "src/LibsGlobais/scDespachanteComandos/scDespachanteComandos.h"
#include "src/LibsGlobais/scBotaoMultifuncao/scBotaoMultifuncao.h"
#include "src/LibsGlobais/scEncoderRotativo/scEncoderRotativo.h"

// ==========================================
// BIBLIOTECAS LOCAIS (Módulo 2)
// ==========================================
#include "src/Libs/scGestorReles/scGestorReles.h"
#include "src/Libs/scWatchfacesM2/scWatchfacesM2.h"
#include "src/Libs/scControladorMenu/scControladorMenu.h"
#include "src/Libs/scGestorAgendamentos/scGestorAgendamentos.h"
#include "src/Libs/scMonitorSaudeM2/scMonitorSaudeM2.h"
#include "src/Libs/scTelemetriaM2/scTelemetriaM2.h"

// ==========================================
// MAPEAMENTO DE HARDWARE (PINOUT ESP-12F)
// ==========================================
#define PIN_R1         4
#define PIN_R2         16
#define PIN_SDA        12
#define PIN_SCL        13
#define PIN_ENC_A      14
#define PIN_ENC_B      3   // RX0
#define PIN_ENC_SW     0
#define PIN_BTN1       2
#define PIN_BTN2       5
#define PIN_BUZZER     15

// MAC Address do HUB Central (Exemplo Padrão)
const uint8_t MAC_HUB[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x33};

// ==========================================
// INSTÂNCIA DE OBJETOS
// ==========================================
scLogger logger;
scArmazenamentoLocal armazenamento;
scAvisosSonoros buzzer(PIN_BUZZER);
scGestorDisplay display;
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2_obj(U8G2_R0, U8X8_PIN_NONE);

scRelogioSincronizado relogio;
scConfigOTA ota;
scMQTTLib mqtt("admin", "admin123");
scTransceptorESPNow transceptor;
scGestorRede rede;
scDespachanteComandos despachante;

scEncoderRotativo encoder(PIN_ENC_A, PIN_ENC_B);
scBotaoMultifuncao botaoEncoder(PIN_ENC_SW);

// Botões físicos de atalho para os relés
scBotaoMultifuncao botao1(PIN_BTN1);
scBotaoMultifuncao botao2(PIN_BTN2);

scGestorReles reles;
scWatchfacesM2 watchfaces;
scControladorMenu controladorMenu;
scGestorAgendamentos agendamentos;
scMonitorSaudeM2 saudeM2;
scTelemetriaM2 telemetriaM2;

// ==========================================
// VARIÁVEIS GLOBAIS DE CONTROLE
// ==========================================
uint32_t tsUltimaTelemetria = 0;
#define INTERVALO_TELEMETRIA_MS 60000 // Ping de saúde a cada 60s

// ==========================================
// CALLBACKS DE HARDWARE E REDE
// ==========================================

// Callbacks para os botões físicos
void AlternarRele1() {
    bool estadoAtual = reles.getEstadoRele(1);
    if (reles.setEstadoRele(1, !estadoAtual)) {
        buzzer.tocar(BIP_CURTO);
        telemetriaM2.despacharTelemetriaM2();
    } else {
        buzzer.tocar(SIRENE_EMERGENCIA); // Bloqueado!
    }
}

void AlternarRele2() {
    bool estadoAtual = reles.getEstadoRele(2);
    if (reles.setEstadoRele(2, !estadoAtual)) {
        buzzer.tocar(BIP_CURTO);
        telemetriaM2.despacharTelemetriaM2();
    } else {
        buzzer.tocar(SIRENE_EMERGENCIA);
    }
}

// Função engatilhada pelo Despachante ao chegar um JSON criptografado
void CallbackTratarComandosM2(const char* cmd, JsonVariant args) {
    if (strcmp(cmd, "set_rele") == 0) {
        if (args.containsKey("id") && args.containsKey("estado")) {
            // Tenta ligar fisicamente a carga
            bool sucesso = reles.setEstadoRele(args["id"].as<int>(), args["estado"].as<bool>());
            if (sucesso) {
                buzzer.tocar(BIP_CURTO);
                telemetriaM2.despacharTelemetriaM2(); // Feedback instantaneo (Ping on Demand)
            } else {
                // Ação foi bloqueada pela scGestorReles (Provavel Regra de Gás de Bloqueio em vigor)
                buzzer.tocar(SIRENE_EMERGENCIA);
            }
        }
    }
    else if (strcmp(cmd, "configurar_rele") == 0) {
        if (args.containsKey("id")) {
            int id = args["id"].as<int>();
            float pot_w = args.containsKey("pot_w") ? args["pot_w"].as<float>() : 0.0;
            int fb_gas = args.containsKey("fb_gas") ? args["fb_gas"].as<int>() : 1;
            int ret_pwr = args.containsKey("ret_pwr") ? args["ret_pwr"].as<int>() : 3;
            
            reles.configurarRele(id, pot_w, fb_gas, ret_pwr);
            buzzer.tocar(BIP_LONGO); // Salvo na flash
        }
    }
    else if (strcmp(cmd, "status_peer") == 0) {
        // O Hub avisa que a placa alvo morreu/ressuscitou
        if (args["tipo"] == "M1") {
            bool online = (args["status"] != "inativo");
            controladorMenu.notificarStatusPeer("M1", online ? "online" : "inativo");
            
            // Se o M1 morrer, revoga a flag local de gas pra nao travar o rele para sempre
            if (!online) {
                reles.acionarEmergenciaGas(false);
            }
        }
    }
}

// Callback de Rádio MESH/Broadcast: Permite o M2 escutar os gritos de socorro e dados do M1 (Forno)
void CallbackEscutaPromiscua(const char* macOrigem, const char* payload) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error && doc["tipo"] == "M1") {
        // Interceptou a telemetria do Forno (M1). Pega a temperatura e joga pra UI OLED!
        if (doc["dados"].containsKey("temp")) {
            telemetriaM2.setTempM1(doc["dados"]["temp"].as<float>());
        }
        
        // Avaliação Critica de Rede (P2P Mesh): Gatilho Direto de Gás
        if (doc["dados"].containsKey("gas")) {
            int gas = doc["dados"]["gas"].as<int>();
            
            if (gas > 1000) { // Limiar Critico (Pode ser puxado da config depois)
                // Engatilha defesa de hardware M2 baseada nas escolhas do usuario (Ex: Cortar Geladeira ou Ligar Exaustor)
                reles.acionarEmergenciaGas(true);
                watchfaces.desenharAlertaGasTelaCheia();
                buzzer.tocar(SIRENE_EMERGENCIA); // Acorda a casa!
            } else {
                reles.acionarEmergenciaGas(false); // Fim da crise
            }
        }
    }
}

// ==========================================
// SETUP (INJEÇÃO DE DEPENDÊNCIAS EM CASCATA)
// ==========================================
void setup() {
    Serial.begin(115200);
    delay(100);

    // 1. Logger nasce primeiro para blindar o "Fail Fast"
    logger.inicializar(true); 
    logger.info("BOOT", "=== Iniciando Módulo 2 (Relés & Encoder) ===");

    // 2. I2C Wire mapeado manualmente no NodeMCU (Soft-I2C se necessario)
    Wire.begin(PIN_SDA, PIN_SCL);

    // 3. Sistema de Vida e Hardware Base
    armazenamento.inicializar();
    
    saudeM2.inicializarLocal(&logger, 0x3C);
    saudeM2.inicializar(5, &armazenamento); // Watchdog brutal de 5s para loops travados
    
    // 4. Hardware Local Simples
    buzzer.inicializar();
    u8g2_obj.begin();
    display.inicializar(&u8g2_obj);
    encoder.inicializar();
    botaoEncoder.inicializar();
    
    botao1.inicializar();
    botao1.setAoClicar(AlternarRele1);
    
    botao2.inicializar();
    botao2.setAoClicar(AlternarRele2);

    // 5. Setup de Rede e Nuvem
    rede.inicializar("REDE", "SENHA123", &logger);
    rede.configurarComoHub(false); 
    
    // 6. Inteligência e Motores do M2
    reles.inicializar(&logger, &armazenamento, &relogio, PIN_R1, PIN_R2);
    agendamentos.inicializar(&logger, &relogio, &armazenamento, &reles);
    
    watchfaces.inicializar(&logger, &display, &relogio, &reles, &telemetriaM2);
    controladorMenu.inicializar(&logger, &display, &encoder, &botaoEncoder, &reles, &watchfaces, &armazenamento);

    telemetriaM2.inicializar(&mqtt, &logger, &saudeM2, &rede, &transceptor, &reles, MAC_HUB);
    
    // O despachante é o roteador principal do arquivo raiz
    despachante.inicializar(WiFi.macAddress(), &relogio, &ota, &logger, &armazenamento, &transceptor);
    despachante.registrarCallbackLocal(CallbackTratarComandosM2);
    
    // Manda o despachante entregar qualquer pacote captado no ar (Mesh passiva)
    despachante.registrarCallbackPromiscuo(CallbackEscutaPromiscua);
    
    logger.info("BOOT", "Boot finalizado. Entrando na esteira Non-Blocking.");
    buzzer.tocar(BIP_DUPLO); // Feedback Auditivo de Sucesso
}

// ==========================================
// LOOP PRINCIPAL (RODA A ~5.000 Hz)
// ==========================================
void loop() {
    // A cada giro, avisa o Watchdog Timer que o sistema respirou
    saudeM2.alimentarWatchdog();

    // Motores de Fundo (Conexão e I/O)
    rede.atualizar();
    if (!rede.estaEmFallback()) {
        mqtt.manterConexao(); 
    }
    // transceptor.atualizar(); // Atualiza fila do ESP-NOW
    
    // UI e Atores Físicos
    botaoEncoder.atualizar();
    botao1.atualizar();
    botao2.atualizar();
    
    controladorMenu.loop();
    reles.loop();
    agendamentos.loop();
    
    // Varreduras Proativas (Fail-Fast I2C)
    saudeM2.verificarIntegridadeLocal();
    
    // Telemetria Agendada (Batimento Cardíaco / Heartbeat)
    if (millis() - tsUltimaTelemetria >= INTERVALO_TELEMETRIA_MS) {
        telemetriaM2.despacharTelemetriaM2();
        tsUltimaTelemetria = millis();
    }
}
