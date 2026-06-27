#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// Globais (Ecossistema / Infraestrutura)
#include "src/LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "src/LibsGlobais/scAvisosSonoros/scAvisosSonoros.h"
#include "src/LibsGlobais/scDespachanteComandos/scDespachanteComandos.h"
#include "src/LibsGlobais/scGestorDisplay/scGestorDisplay.h"
#include "src/LibsGlobais/scGestorRede/scGestorRede.h"
#include "src/LibsGlobais/scLogger/scLogger.h"
#include "src/LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "src/LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "src/LibsGlobais/scTransceptorESPNow/scTransceptorESPNow.h"

// Locais (Módulo 1 / Negócio e Sensores)
#include "src/Libs/scGestorAlarmesM1/scGestorAlarmesM1.h"
#include "src/Libs/scGestorReleM1/scGestorReleM1.h"
#include "src/Libs/scLeitorTermicoM1/scLeitorTermicoM1.h"
#include "src/Libs/scMonitorGasM1/scMonitorGasM1.h"
#include "src/Libs/scMonitorSaudeM1/scMonitorSaudeM1.h"
#include "src/Libs/scTelemetriaM1/scTelemetriaM1.h"
#include "src/Libs/scWatchfacesM1/scWatchfacesM1.h"

// Pinos - ESP-12F
#define PINO_GAS A0
#define PINO_TERMICO 13
#define PINO_RELE_SSR 12
#define PINO_BUZZER 14
#define PINO_SDA 4
#define PINO_SCL 5

// ====================================================================
// INSTÂNCIAS GLOBAIS (Singletons do Módulo 1)
// ====================================================================
scLogger logger;
scGestorRede gestorRede;
// Instanciação I2C padrão para ESP8266 (OLED 0.96")
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE,
                                         PINO_SCL, PINO_SDA);
scGestorDisplay display;
scTransceptorESPNow transceptor;
scMQTTLib mqtt("usuario_mqtt", "senha_mqtt"); //
scRelogioSincronizado relogio;
scAvisosSonoros avisosSonoros(PINO_BUZZER);
scArmazenamentoLocal armazenamento;
scDespachanteComandos despachante;

scMonitorGasM1 monitorGas;
scLeitorTermicoM1 leitorTermico;
scGestorReleM1 gestorRele;
scGestorAlarmesM1 gestorAlarmes;
scMonitorSaudeM1 monitorSaude;
scWatchfacesM1 watchfaces;
scTelemetriaM1 telemetria;

// MAC do Hub (Fallback ESP-NOW) e Módulo 2 (Trigger P2P)
const uint8_t MAC_HUB[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
const uint8_t MAC_MODULO2[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

// ====================================================================
// CALLBACKS (Arquitetura Event-Driven Desacoplada)
// ====================================================================

// Quando o MQ-2 detecta alteração brusca de Estado
void onVazamentoGas(bool detectado, uint16_t nivelAtual) {
  gestorAlarmes.setEstadoVazamento(detectado);
  gestorRele.acionarEmergencia(detectado); // Corta ou libera o relé redundante

  // Anti-SPOF: Comunicação Direta ESP-NOW Edge-Trigger
  String macAlvo = armazenamento.obterValor("peer_M2");
  if (macAlvo == "") macAlvo = "11:22:33:44:55:66"; // MAC de Teste

  uint8_t macBytes[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
  if (macAlvo.length() == 17) {
    for (int i = 0; i < 6; i++) {
        macBytes[i] = (uint8_t)strtoul(macAlvo.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
    }
  }

  if (detectado) {
    logger.erro("M1_CORE",
                "VAZAMENTO CONFIRMADO! Atirando trigger P2P para o Modulo 2!");
    String payload = "{\"mac_destino\":\"" + macAlvo + "\",\"tipo\":\"M1\",\"dados\":{\"gas\": 1500}}";
    transceptor.enviarPacote(macBytes, payload);
  } else {
    logger.info("M1_CORE", "Gas estabilizado. Enviando trigger P2P de alivio.");
    String payload = "{\"mac_destino\":\"" + macAlvo + "\",\"tipo\":\"M1\",\"dados\":{\"gas\": 0}}";
    transceptor.enviarPacote(macBytes, payload);
  }
}

// Quando o DS18B20 entende que o forno ligou (por Derivada ou Absoluto)
void onEstadoForno(bool ligado) { gestorAlarmes.setEstadoForno(ligado); }

void CallbackTratarComandosM1(const char* cmd, JsonVariant args) {
    if (strcmp(cmd, "configurar_operacao") == 0) {
        if (args.containsKey("tela_idle")) watchfaces.setModoTela(args["tela_idle"].as<uint8_t>());
        if (args.containsKey("sens_gas")) monitorGas.setSensibilidade(args["sens_gas"].as<uint16_t>());
        
        if (args.containsKey("alm_prep") || args.containsKey("alm_crit")) {
            uint32_t preparoAtual = gestorAlarmes.AlmPrep();
            uint32_t criticoAtual = gestorAlarmes.AlmCrit();
            
            if (args.containsKey("alm_prep")) preparoAtual = args["alm_prep"].as<uint32_t>();
            if (args.containsKey("alm_crit")) criticoAtual = args["alm_crit"].as<uint32_t>();
            
            gestorAlarmes.setAlarmes(preparoAtual, criticoAtual);
        }
    }
    else if (strcmp(cmd, "solicitar_status") == 0) {
        telemetria.despacharTelemetriaM1();
    }
}

// Ponte entre a Interrupção de Hardware do ESP-NOW e o Core
void onDataRecvESPNow(const char *macOrigem, const char *payload) {
  despachante.processarPayload(macOrigem, payload);
}

// ====================================================================
// SETUP (Boot Sequence e Injeção de Dependências)
// ====================================================================
void setup() {
  Serial.begin(115200);

  // 1. Motores de Infraestrutura (Globais)
  logger.inicializar();
  logger.info("M1_BOOT", "Inicializando Modulo 1 - Bare Metal Securitario");

  Wire.begin(PINO_SDA, PINO_SCL);
  u8g2.begin();
  display.inicializar(&u8g2);

  // Usando credenciais padrão de fábrica (Modo de Teste) até que o Hub (M3)
  // assuma e provisione a rede correta
  gestorRede.inicializar("REDE", "SENHA123", &logger);
  gestorRede.injetarMQTT(&mqtt); // Para validacao de Roteador Zumbi
  
#if defined(ESP8266)
  String macDestaPlaca = WiFi.macAddress();
#elif defined(ESP32)
  String macDestaPlaca = WiFi.macAddress();
#else
  String macDestaPlaca = "M1_DEV";
#endif

  armazenamento.inicializar();
  despachante.inicializar(macDestaPlaca, &relogio, nullptr, &logger, &armazenamento, &transceptor);
  despachante.registrarCallbackLocal(CallbackTratarComandosM1);

  transceptor.inicializar(&logger);
  transceptor.definirRecebimento(onDataRecvESPNow);

  mqtt.iniciar();
  relogio.inicializar();
  avisosSonoros.inicializar();

  // 2. Motores de Negócio (Camada Física Local)
  monitorGas.inicializar(&logger, PINO_GAS);
  monitorGas.setCallbackVazamento(onVazamentoGas);

  leitorTermico.inicializar(&logger, PINO_TERMICO);
  leitorTermico.setCallbackEstadoForno(onEstadoForno);

  gestorRele.inicializar(&logger, PINO_RELE_SSR);
  gestorAlarmes.inicializar(&logger, &avisosSonoros);

  monitorSaude.inicializarLocal(&logger, leitorTermico.obterSensorDS(), PINO_GAS);
  monitorSaude.inicializar(5, &armazenamento);
  watchfaces.inicializar(&logger, &display, &relogio, &monitorGas,
                         &leitorTermico);

  telemetria.inicializar(&mqtt, &logger, &monitorSaude, &gestorRede,
                         &transceptor, &monitorGas, &leitorTermico,
                         &gestorAlarmes, MAC_HUB);

  logger.info("M1_BOOT",
              "Injecao de Dependencias concluida. Partindo Super Loop!");
}

// ====================================================================
// LOOP PRINCIPAL (Bare Metal C++)
// ====================================================================
void loop() {
  // 1. Manutenção de Conectividade e Tempo
  gestorRede.atualizar();

  if (!gestorRede.estaEmFallback()) {
    mqtt.manterConexao();
    relogio.atualizar();

    // Verifica se há ordens da Nuvem
    if (mqtt.temComando()) {
      despachante.processarPayload("HUB", mqtt.obterComando().c_str());
    }
  }

  avisosSonoros.atualizar();

  // 2. Leitura Crítica Contínua (Não-bloqueante)
  monitorGas.processar();
  leitorTermico.processar();

  // 3. Motores de Reação, Tolerância a Falhas e UI
  gestorAlarmes.processar();
  monitorSaude.verificarIntegridadeLocal();
  watchfaces.atualizar();

  // 4. Timer de Telemetria (Dispara o empacotador a cada 30 segundos)
  static uint32_t ultimoDisparoTelemetria = 0;
  if (millis() - ultimoDisparoTelemetria >= 30000) {
    ultimoDisparoTelemetria = millis();
    telemetria.despacharTelemetriaM1();
  }

  // Princípio do SDK da Espressif: Liberar a CPU para manter a pilha WiFi
  // rodando lisa
  yield();
}
