#include "scSaudeHardware.h"
#include "../scArmazenamentoLocal/scArmazenamentoLocal.h"

#define HORA_REBOOT 3   // 3:00 AM
#define MINUTO_REBOOT 0 // 3:00 AM

scSaudeHardware::scSaudeHardware()
    : _uptimeInicial(0), _uptimeAcumulado(0), _armazenamento(nullptr) {}

void scSaudeHardware::inicializar(uint32_t timeoutWatchdogSegundos,
                                  scArmazenamentoLocal *armazenamento) {
  _uptimeInicial = millis();
  _armazenamento = armazenamento;

  // Logica da Flag Efemera: Verifica se o reboot foi preventivo
  if (_armazenamento != nullptr) {
    String salvo = _armazenamento->obterValor("upt_estavel");
    if (salvo.length() > 0) {
      _uptimeAcumulado = salvo.toInt();

      // Apaga para que em caso de pane eletrica real a flag nao exista no boot
      _armazenamento->removerChave("upt_estavel");
      _armazenamento->commitarAlteracoes();
    } else {
      // Faltou luz ou o WDT cortou. Zera o historico estavel.
      _uptimeAcumulado = 0;
    }
  }

#if defined(ESP8266)
  // O ESP8266 ja roda sob um WDT de software rigoroso por padrao do Core do
  // Arduino. Habilitar via API garante que ele proteja contra timeouts grandes.
  // O argumento ideal para o ESP8266 seria uma macro como WDTO_8S, mas para
  // simplificar:
  ESP.wdtEnable(timeoutWatchdogSegundos * 1000);
#elif defined(ESP32)
  // No ESP32 (Arduino-FreeRTOS), configura o Task WDT.
  // O 'true' indica que irá causar um PANIC (reboot) se houver timeout.
  // (Nota: Em núcleos ESP32 v3.0+ a assinatura muda, mas para v2.x isso é
  // perfeito)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
  esp_task_wdt_config_t config = {
      .timeout_ms = timeoutWatchdogSegundos * 1000,
      .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
      .trigger_panic = true,
  };
  esp_task_wdt_init(&config);
#else
  esp_task_wdt_init(timeoutWatchdogSegundos, true);
#endif

  // Adiciona a tarefa atual (o loop do Arduino) na lista de vigia do WDT.
  esp_task_wdt_add(NULL);
#endif
}

void scSaudeHardware::alimentarWatchdog() {
#if defined(ESP8266)
  ESP.wdtFeed();
#elif defined(ESP32)
  esp_task_wdt_reset();
#endif
}

uint32_t scSaudeHardware::getRamLivre() { return ESP.getFreeHeap(); }

uint32_t scSaudeHardware::getUptimeSessaoSegundos() {
  return millis() / 1000; // Tenho tratativa para estouro de a cada ~50 dias
}

uint32_t scSaudeHardware::getUptimeEstavelSegundos() {
  return _uptimeAcumulado + getUptimeSessaoSegundos();
}

void scSaudeHardware::verificarRebootPreventivo(uint8_t horaAtual,
                                                uint8_t minutoAtual) {
  static bool ReiniciouHoje = false;

  // Reboot programado para as 03:00 da manha
  if (horaAtual == HORA_REBOOT && minutoAtual == MINUTO_REBOOT) {
    if (!ReiniciouHoje) {
      ReiniciouHoje = true;

      // Logica Efemera: Salva o uptime minutos antes da morte
      if (_armazenamento != nullptr) {
        uint32_t totalAgora = getUptimeEstavelSegundos();
        _armazenamento->salvarChaveValor("upt_estavel", String(totalAgora));
        _armazenamento->commitarAlteracoes();
      }

      // Realiza o reinicio preventivo da placa
#if defined(ESP8266)
      ESP.restart();
#elif defined(ESP32)
      ESP.restart();
#endif
    }
  } else {
    ReiniciouHoje = false;
  }
}

float scSaudeHardware::getTemperaturaInterna() {
#if defined(ESP8266)
  // O ESP8266 nao possui um sensor interno de temperatura exposto na API padrao
  return 0.0;
#elif defined(ESP32)
  // A funcao temperatureRead() le o sensor nativo no interior do die do ESP32.
  // Muito util no Módulo 3 (Hub) para prevenir Thermal Runaway
  return temperatureRead();
#endif
}
