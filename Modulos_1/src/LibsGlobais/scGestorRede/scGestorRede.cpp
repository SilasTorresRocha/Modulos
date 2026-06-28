#include "scGestorRede.h"
#include "../scMQTTLib/scMQTTLib.h"
#include "../scLogger/scLogger.h"

scGestorRede::scGestorRede() {
  _estado = REDE_WIFI_CONECTANDO;
  _ultimoCheckTempo = 0;
  _tempoInicioConexao = 0;
  _tempoUltimoScanCanal = 0;
  _canalEspNow = 1;
  _ssid = "";
  _password = "";
  _eHub = false;
  _mqtt = nullptr;
  _logger = nullptr;
}

void scGestorRede::inicializar(const char *ssid, const char *password, scLogger* logger) {
  _ssid = ssid;
  _password = password;
  _logger = logger;

  // O Hub (M3) opera como AP_STA (Hospedeiro ESP-NOW + Cliente Wi-Fi)
  // Os nós (M1, M2) operam puramente como STA
  if (_eHub) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_STA);
  }

  WiFi.disconnect();

  tentarReconectarWiFi();
}

void scGestorRede::configurarComoHub(bool eHub) { _eHub = eHub; }

void scGestorRede::injetarMQTT(scMQTTLib *mqtt) { _mqtt = mqtt; }

void scGestorRede::atualizar() {
  uint32_t tempoAtual = millis();

  switch (_estado) {
  case REDE_WIFI_CONECTANDO:
    // Tenta conectar no Wi-Fi físico por 15 segundos.
    if (WiFi.status() == WL_CONNECTED) {
      // Ao invés de ir pro estado Conectado direto (que causaria falso positivo
      // no MQTT), nós avançamos para a fase de Validação de Internet!
      _estado = REDE_VALIDANDO_INTERNET;
      _tempoInicioConexao =
          tempoAtual; // Reaproveitamos o timer para contar 10s
    } else if ((uint32_t)(tempoAtual - _tempoInicioConexao) > 15000) {
      ativarESPNow();
    }
    break;

  case REDE_VALIDANDO_INTERNET:
    // O Wi-Fi fisicamente conectou. Agora damos 10 segundos para o handshake
    // TCP da scMQTTLib acontecer e provar que há saída real para a internet.
    if (_mqtt != nullptr && _mqtt->internetDisponivel()) {
      // Sucesso Absoluto! Camada 2 e Camada 7 ativas.
      _estado = REDE_WIFI_CONECTADO;
    } else if ((uint32_t)(tempoAtual - _tempoInicioConexao) > 10000) {
      // Estourou os 10 segundos e o MQTT não conectou. É um Roteador Zumbi.
      ativarESPNow();
    } else if (WiFi.status() != WL_CONNECTED) {
      // Wi-Fi desabou durante a tentativa
      ativarESPNow();
    }
    break;

  case REDE_WIFI_CONECTADO:
    // Checa se o roteador ainda está vivo a cada 2 segundos para não alugar a
    // CPU
    if ((uint32_t)(tempoAtual - _ultimoCheckTempo) >= 2000) {
      _ultimoCheckTempo = tempoAtual;

      bool falhaFisica = (WiFi.status() != WL_CONNECTED);

      // O MQTT pode ter caído mesmo que o Wi-Fi esteja WL_CONNECTED (Roteador
      // Zumbi)
      bool falhaLogica = (_mqtt != nullptr && !_mqtt->internetDisponivel());

      if (falhaFisica || falhaLogica) {
        // Caiu a internet (Física ou Lógica). Aciona os protocolos de
        // emergencia!
        ativarESPNow();
      }
    }
    break;

  case REDE_FALLBACK_ATIVO:
    // No modo de Sobrevivência/Fallback, precisamos fazer duas coisas:

    // 1. Escanear os canais do ESP-NOW buscando a base (Módulo 3) a cada 500ms
    if ((uint32_t)(tempoAtual - _tempoUltimoScanCanal) >= 500) {
      _tempoUltimoScanCanal = tempoAtual;
      escanearCanaisESPNow();
    }

    // 2. Tentar reviver o Wi-Fi periodicamente a cada 60 segundos
    if ((uint32_t)(tempoAtual - _ultimoCheckTempo) >= 60000) {
      _ultimoCheckTempo = tempoAtual;
      tentarReconectarWiFi();
    }
    break;
  }
}

bool scGestorRede::estaEmFallback() { return _estado == REDE_FALLBACK_ATIVO; }

int scGestorRede::obterRssi() {
  if (_estado == REDE_WIFI_CONECTADO) {
    return WiFi.RSSI();
  }
  // Retorna vazio sonoro se não tiver conectado no AP físico
  return -100;
}

void scGestorRede::relatarFalhaDeInternet() {
  // A camada de rede (Camada 2 OSI) pode estar OK (Roteador ligado),
  // mas a Camada de Aplicação (MQTT/Internet) falhou.
  // O método relatarFalhaDeInternet agora é apenas um atalho opcional,
  // já que o _callbackInternet faz o trabalho pesado de forma automática.
  if (_estado == REDE_WIFI_CONECTADO) {
    ativarESPNow();
  }
}

void scGestorRede::tentarReconectarWiFi() {
  _estado = REDE_WIFI_CONECTANDO;
  _tempoInicioConexao = millis();

  if (_logger) _logger->info("REDE", "Tentando reconectar ao Wi-Fi...");

  // Chama o driver do core do ESP para buscar a rede em background.
  // Isso NÃO trava o loop.
  WiFi.begin(_ssid, _password);
}

void scGestorRede::ativarESPNow() {
  if (_estado != REDE_FALLBACK_ATIVO && _logger) {
      _logger->warn("REDE", "Falha critica de conexao detectada. Entrando em Fallback ESP-NOW.");
  }
  _estado = REDE_FALLBACK_ATIVO;
  _ultimoCheckTempo = millis();
  _tempoUltimoScanCanal = millis();

  // O Hub é a rocha do sistema. Ele nunca solta o osso do Wi-Fi e nunca derruba
  // o ESP-NOW. Somente os módulos escravos (M1, M2) desconectam do roteador
  // zumbi para focar no rádio.
  if (!_eHub) {
    WiFi.disconnect();
  }
}

void scGestorRede::escanearCanaisESPNow() {
  // Se for o Hub, ele IGNORA o scan. O Hub dita o canal, os outros seguem.
  if (_eHub)
    return;

  // O módulo órfão varre o espectro como um rádio amador até achar a base.

  _canalEspNow++;
  if (_canalEspNow > 13) {
    _canalEspNow = 1; // Reseta após a banda de 2.4GHz
  }

  if (_logger) _logger->info("REDE", "Buscando Hub M3 via ESP-NOW no canal: " + String(_canalEspNow));

#if defined(ESP8266)
  wifi_set_channel(_canalEspNow);
#elif defined(ESP32)
  esp_wifi_set_channel(_canalEspNow, WIFI_SECOND_CHAN_NONE);
#endif
}
