#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>

// Globais (Ecossistema / Infraestrutura)
#include "src/LibsGlobais/scLogger/scLogger.h"
#include "src/LibsGlobais/scGestorRede/scGestorRede.h"
#include "src/LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "src/LibsGlobais/scTransceptorESPNow/scTransceptorESPNow.h"
#include "src/LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "src/LibsGlobais/scAvisosSonoros/scAvisosSonoros.h"

// Locais (Módulo 3 / Roteamento e Hub)
#include "src/Libs/scRTCFisicoM3/scRTCFisicoM3.h"
#include "src/Libs/scGestorSDCardM3/scGestorSDCardM3.h"
#include "src/Libs/scMonitorSaudeM3/scMonitorSaudeM3.h"
#include "src/Libs/scRoteadorBridgeM3/scRoteadorBridgeM3.h"
#include "src/Libs/scTelemetriaM3/scTelemetriaM3.h"
#include "src/Libs/scWatchfacesM3/scWatchfacesM3.h"

// ====================================================================
// MAPEAMENTO FÍSICO ESP32-S2 
// ====================================================================
#define PINO_I2C_SDA 8
#define PINO_I2C_SCL 9
#define PINO_SPI_SCK 36
#define PINO_SPI_MISO 37
#define PINO_SPI_MOSI 35
#define PINO_SD_CS 10
#define PINO_BUZZER 11 

// ====================================================================
// INSTÂNCIAS (Singletons)
// ====================================================================
scLogger logger;
scGestorRede gestorRede;
scTransceptorESPNow transceptor;
scMQTTLib mqtt("usuario_mqtt", "senha_mqtt");
scRelogioSincronizado relogio;
scAvisosSonoros avisosSonoros(PINO_BUZZER);

scRTCFisicoM3 rtc;
scGestorSDCardM3 sdCard;
scMonitorSaudeM3 monitorSaude;
scRoteadorBridgeM3 roteador;
scTelemetriaM3 telemetria;
scWatchfacesM3 watchfaces;

SPIClass spiShared(FSPI); // Barramento SPI Compartilhado (TFT, Touch, SD)

String macPlaca;

// Flags Assíncronas (Thread-Safe)
volatile bool flagEmergenciaGas = false;
volatile uint32_t ultimoTickLVGL = 0;

// ====================================================================
// HANDLES DE TAREFAS (FreeRTOS)
// ====================================================================
TaskHandle_t TaskRedeHandle = NULL;
TaskHandle_t TaskTelaHandle = NULL;

// ====================================================================
// CALLBACKS
// ====================================================================
void onDataRecvESPNow(const char *macOrigem, const char *payload) {
    // Interrompe qualquer coisa e passa a bola para o roteador central
    roteador.rotearPacoteDeESPNowParaNuvem(macOrigem, payload);
    
    // strstr é mais rapido e seguro que converter para String no callback da interrupção do rádio
    if (strstr(payload, "\"gas\":") != NULL) {
        flagEmergenciaGas = true;
    }
}

// ====================================================================
// TAREFA 1: REDE E ROTEAMENTO (ALTA PRIORIDADE - SINGLE CORE)
// ====================================================================
void TaskRedeRoteamento(void *pvParameters) {
    logger.info("RTOS", "Tarefa de Roteamento Iniciada (Core " + String(xPortGetCoreID()) + ")");

    uint32_t ultimoDisparoTelemetria = 0;

    for (;;) {
        // 1. Manutencao de Vida
        gestorRede.atualizar();
        avisosSonoros.atualizar();
        rtc.atualizar();

        // 2. Conexao Nuvem
        if (!gestorRede.estaEmFallback()) {
            mqtt.manterConexao();
            relogio.atualizar();
            
            // Verifica ordens descendo da nuvem
            if (mqtt.temComando()) {
                roteador.rotearPacoteDaNuvemParaESPNow(mqtt.obterComando());
            }
        }
        
        // 3. Esvaziamento de Fila Offline
        roteador.atualizar();
        
        // 4. Saude e Telemetria do proprio Hub
        monitorSaude.setWatchdogLVGL(millis() - ultimoTickLVGL > 5000); // 5s sem piscar = travado
        monitorSaude.verificarIntegridadeLocal();
        
        if (millis() - ultimoDisparoTelemetria >= 60000) {
            ultimoDisparoTelemetria = millis();
            telemetria.despacharTelemetriaM3();
            watchfaces.atualizarStatusWiFi(!gestorRede.estaEmFallback());
            watchfaces.atualizarTemperatura(rtc.obterTemperaturaRTC());
        }

        // 5. Processamento seguro de Flags Assincronas (Evita colisão com LVGL)
        if (flagEmergenciaGas) {
            watchfaces.atualizarStatusGas(true);
            avisosSonoros.tocar(SIRENE_EMERGENCIA);
            flagEmergenciaGas = false;
        }

        // Delay minimo para o Watchdog do FreeRTOS nao reiniciar a placa
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ====================================================================
// TAREFA 2: MOTOR GRÁFICO LVGL (BAIXA PRIORIDADE - SINGLE CORE)
// ====================================================================
void TaskAtualizarTela(void *pvParameters) {
    logger.info("RTOS", "Tarefa Grafica Iniciada (Core " + String(xPortGetCoreID()) + ")");

    for (;;) {
        ultimoTickLVGL = millis(); // "Estou vivo!"
        
        // Se a tarefa LVGL travar por lentidão do SD, o Watchdog não será reiniciado. 
        // Como ela tem prioridade MENOR que a rede, o ESP-NOW continuará roteando vazamentos de gás intacto!
        watchfaces.atualizar();
        
        vTaskDelay(pdMS_TO_TICKS(5)); // Engine gráfica exige pequenos yields frequentes
    }
}

// ====================================================================
// SETUP (Boot Sequence)
// ====================================================================
void setup() {
    Serial.begin(115200);

    // 1. Infrastrutura Base
    logger.inicializar();
    logger.info("M3_BOOT", "=== Iniciando HUB CENTRAL (ESP32-S2) ===");

    Wire.begin(PINO_I2C_SDA, PINO_I2C_SCL);
    spiShared.begin(PINO_SPI_SCK, PINO_SPI_MISO, PINO_SPI_MOSI, -1);

    gestorRede.inicializar("REDE_CASA", "SENHA_WIFI", &logger);
    gestorRede.injetarMQTT(&mqtt); 

#if defined(ESP8266) || defined(ESP32)
    macPlaca = WiFi.macAddress();
#else
    macPlaca = "M3_HUB";
#endif

    // 2. Transceptores
    transceptor.inicializar(&logger);
    transceptor.definirRecebimento(onDataRecvESPNow);
    mqtt.iniciar();

    // 3. SD e Relogio Físico
    sdCard.inicializar(&logger, PINO_SD_CS, &spiShared);
    rtc.inicializar(&logger, &relogio);
    relogio.inicializar();
    avisosSonoros.inicializar();

    // 4. UI e Roteamento
    monitorSaude.inicializarLocal(&logger, &rtc, &sdCard);
    roteador.inicializar(&logger, &mqtt, &transceptor, &gestorRede, &sdCard);
    telemetria.inicializar(&logger, &gestorRede, &relogio, &rtc, &monitorSaude, &roteador, macPlaca);
    watchfaces.inicializar(&logger); // Inicia LVGL + TFT_eSPI (DMA)

    logger.info("M3_BOOT", "Injecao completa. Subindo FreeRTOS Tasks!");

    // 5. Orquestração FreeRTOS (O Segredo do ESP32-S2 Single Core)
    // Prioridade Alta (3) para Rede, Baixa (1) para Graficos.
    // Core 0 obrigatoriamente (S2 só tem o core 0).
    
    xTaskCreatePinnedToCore(
        TaskRedeRoteamento,   // Funcao
        "Task_Rede",          // Nome
        10000,                // Stack Size
        NULL,                 // Params
        3,                    // Prioridade (Alta)
        &TaskRedeHandle,      // Handle
        0                     // Core 0 (ESP32-S2)
    );

    xTaskCreatePinnedToCore(
        TaskAtualizarTela,    // Funcao
        "Task_Tela",          // Nome
        8000,                 // Stack Size
        NULL,                 // Params
        1,                    // Prioridade (Baixa)
        &TaskTelaHandle,      // Handle
        0                     // Core 0 (ESP32-S2)
    );

    // O loop vazio. O FreeRTOS ja assumiu o comando.
}

void loop() {
    vTaskDelete(NULL); // Deleta a tarefa padrao do loop, economiza RAM e CPU para as Tasks customizadas
}
