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
#define PIN_ENC_B      5   // Trocado com o BTN2! O pino RX (3) tem conflito fisico com o chip USB!
#define PIN_ENC_SW     0
#define PIN_BTN1       2
#define PIN_BTN2       3   // O Botao 2 foi para o pino RX (Ele tem forca mecanica para dar o curto pro GND)
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
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_obj(U8G2_R0, U8X8_PIN_NONE); // Alterado para SH1106 para remover ruidos na borda

scRelogioSincronizado relogio;
scConfigOTA ota;
scMQTTLib mqtt("silastorres", "010203");
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
// VARIÁVEIS GLOBAIS DE OPERAÇÃO
// ==========================================
int limiarGas = 1000; // Valor padrão, pode ser sobrescrito pelo Backend
int ultimoGasRecebido = 0; // Armazena a última leitura para gatilho instantâneo

// ==========================================
// FUNÇÕES AUXILIARES / CALLBACKS DE HARDWARE
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
    else if (strcmp(cmd, "reset_kwh") == 0) {
        if (args.containsKey("id")) {
            int id = args["id"].as<int>();
            reles.resetarConsumo(id);
            buzzer.tocar(BIP_CURTO);
            logger.info("RELES", "Consumo KWh zerado para o Relé " + String(id));
        }
    }
    else if (strcmp(cmd, "tela_idle") == 0) {
        if (args.containsKey("tela")) {
            // Usa o scControladorMenu para definir e salvar a tela (já que o M2 não tem scGestorTelas independente)
            int tela = args["tela"].as<int>();
            // Hack temporário: O Controlador de Menu do M2 não tem um setter público para a watchface. 
            // Precisaremos modificar o scControladorMenu.cpp/h em seguida.
            controladorMenu.setWatchface(tela);
            buzzer.tocar(BIP_CURTO);
        }
    }
    else if (strcmp(cmd, "agendar_tarefa") == 0) {
        if (args.containsKey("id_agd") && args.containsKey("rele") && args.containsKey("dia") && args.containsKey("hora") && args.containsKey("min") && args.containsKey("acao")) {
            agendamentos.adicionarAgendamento(
                args["id_agd"].as<int>(),
                args["rele"].as<int>(),
                args["dia"].as<String>(),
                args["hora"].as<int>(),
                args["min"].as<int>(),
                args["acao"].as<bool>()
            );
            buzzer.tocar(BIP_CURTO);
        }
    }
    else if (strcmp(cmd, "excluir_agendamento") == 0) {
        if (args.containsKey("id_agd")) {
            agendamentos.excluirAgendamento(args["id_agd"].as<int>());
            buzzer.tocar(BIP_CURTO);
        }
    }
    else if (strcmp(cmd, "configurar_limiar_gas") == 0) {
        if (args.containsKey("limiar")) {
            limiarGas = args["limiar"].as<int>();
            armazenamento.salvarChaveValor("lim_gas", String(limiarGas));
            armazenamento.commitarAlteracoes();
            buzzer.tocar(BIP_LONGO); // Salvo na flash
            logger.info("GAS", "Limiar de Gas atualizado para: " + String(limiarGas));
            
            // Re-avalia imediatamente com a última leitura conhecida!
            if (ultimoGasRecebido > limiarGas) {
                reles.acionarEmergenciaGas(true);
                watchfaces.desenharAlertaGasTelaCheia();
                buzzer.tocar(SIRENE_EMERGENCIA);
            } else {
                reles.acionarEmergenciaGas(false);
            }
        }
    }
    else if (strcmp(cmd, "repassar_telemetria") == 0) {
        if (args["tipo"] == "M1") {
            if (args.containsKey("temp")) {
                telemetriaM2.setTempM1(args["temp"].as<float>());
            }
            
            bool emergenciaForno = false;
            if (args.containsKey("t_forno") && args.containsKey("alm_crit")) {
                int tForno = args["t_forno"].as<int>();
                int almCrit = args["alm_crit"].as<int>();
                if (almCrit > 0 && tForno >= almCrit) {
                    emergenciaForno = true;
                }
            }
            
            if (args.containsKey("gas")) {
                ultimoGasRecebido = args["gas"].as<int>();
            }
            
            if (ultimoGasRecebido > limiarGas || emergenciaForno) { 
                reles.acionarEmergenciaGas(true);
                watchfaces.desenharAlertaGasTelaCheia();
                buzzer.tocar(SIRENE_EMERGENCIA);
            } else {
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
        
        bool emergenciaForno = false;
        if (doc["dados"].containsKey("t_forno") && doc["dados"].containsKey("alm_crit")) {
            int tForno = doc["dados"]["t_forno"].as<int>();
            int almCrit = doc["dados"]["alm_crit"].as<int>();
            if (almCrit > 0 && tForno >= almCrit) {
                emergenciaForno = true;
            }
        }

        // Avaliação Critica de Rede (P2P Mesh): Gatilho Direto de Gás ou Esquecimento
        if (doc["dados"].containsKey("gas")) {
            ultimoGasRecebido = doc["dados"]["gas"].as<int>();
        }
        
        if (ultimoGasRecebido > limiarGas || emergenciaForno) { 
            // Engatilha defesa de hardware M2 baseada nas escolhas do usuario (Ex: Cortar Geladeira ou Ligar Exaustor)
            reles.acionarEmergenciaGas(true);
            watchfaces.desenharAlertaGasTelaCheia();
            buzzer.tocar(SIRENE_EMERGENCIA); // Acorda a casa!
        } else {
            reles.acionarEmergenciaGas(false); // Fim da crise
        }
    }
}

// ==========================================
// SETUP (INJEÇÃO DE DEPENDÊNCIAS EM CASCATA)
// ==========================================
void setup() {
    // Permite que o GPIO 3 (RX) seja usado como INPUT_PULLUP pelo Encoder
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
    delay(100);

    // 1. Logger nasce primeiro para blindar o "Fail Fast"
    logger.inicializar(true); 
    logger.info("BOOT", "=== Iniciando Módulo 2 (Relés & Encoder) ===");

    // 2. I2C Wire mapeado manualmente no NodeMCU
    Wire.begin(PIN_SDA, PIN_SCL);
    Wire.setClock(100000); // 100kHz para máxima estabilidade (evita travamentos)

    // 3. Sistema de Vida e Hardware Base
    armazenamento.inicializar();
    //armazenamento.inicializar(&logger);

    String valGas = armazenamento.obterValor("lim_gas");
    if (valGas != "") limiarGas = valGas.toInt();

    saudeM2.inicializarLocal(&logger, 0x3C);
    saudeM2.inicializar(5, &armazenamento); // Watchdog brutal de 5s para loops travados
    
    // Inicia a base de tempo para garantir que a watchface tenha a hora (GMT-3 = -10800s)
    relogio.inicializar(-10800);
    
    // 4. Hardware Local Simples
    buzzer.inicializar();
    u8g2_obj.setBusClock(100000); // Forca 100kHz no driver do OLED
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
    
    mqtt.iniciar(); // Previne o Exception 28
    rede.injetarMQTT(&mqtt); // Previne crashes de logica de rede sem MQTT
    
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
    if (!rede.estaEmFallback() && WiFi.status() == WL_CONNECTED) {
        mqtt.manterConexao(); 
        
        // Puxa as mensagens recebidas via MQTT e entrega para o Cérebro processar
        if (mqtt.temComando()) {
            despachante.processarPayload("MQTT_SERVER", mqtt.obterComando().c_str());
        }
    }
    // transceptor.atualizar(); // Atualiza fila do ESP-NOW
    
    // UI e Atores Físicos
    relogio.atualizar(); // Coleta NTP em background quando o WiFi conectar
    buzzer.atualizar(); // Essencial para parar o som de alerta inicial
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
