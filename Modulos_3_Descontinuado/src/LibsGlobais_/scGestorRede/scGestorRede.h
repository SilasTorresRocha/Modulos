#ifndef SC_GESTOR_REDE_H
#define SC_GESTOR_REDE_H

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#include <esp_wifi.h>
#endif

// FORWARD DECLARATION GLOBAL
class scMQTTLib;
class scLogger;

enum EstadoRede {
  REDE_WIFI_CONECTANDO,
  REDE_VALIDANDO_INTERNET, // Fase de handshake TCP (Evita Race Condition)
  REDE_WIFI_CONECTADO,
  REDE_FALLBACK_ATIVO
};

class scGestorRede {
public:
  scGestorRede();

  // Inicia a gestão de rede informando credenciais e o logger
  void inicializar(const char *ssid, const char *password, scLogger* logger);

  // O motor não-bloqueante que deve rodar no loop() principal
  void atualizar();

  // Retorna se o dispositivo perdeu o Wi-Fi e migrou para ESP-NOW
  bool estaEmFallback();

  // Retorna a força do sinal do roteador (RSSI)
  int obterRssi();

  // Injetor de falha lógica. Permite que o Módulo avise se a internet (MQTT)
  // caiu, mesmo que o Wi-Fi do roteador local continue conectado.
  void relatarFalhaDeInternet();
  // Define se a placa atual é a âncora do ecossistema (Hub Módulo 3)
  // O Hub nunca desliga o ESP-NOW e nunca muda de canal aleatoriamente.
  void configurarComoHub(bool eHub);

  // Injeção Direta da biblioteca MQTT para validar internet (Roteador Zumbi).
  // Usamos Forward Declaration para evitar dependências circulares de Include.
  void injetarMQTT(scMQTTLib *mqtt);

private:
  const char *_ssid;
  const char *_password;

  bool _eHub;
  scMQTTLib *_mqtt;
  scLogger *_logger;

  EstadoRede _estado;
  uint32_t _ultimoCheckTempo;
  uint32_t _tempoInicioConexao;
  uint32_t _tempoUltimoScanCanal;

  uint8_t _canalEspNow;

  // Métodos internos de chaveamento
  void tentarReconectarWiFi();
  void ativarESPNow();
  void escanearCanaisESPNow();
};

#endif // SC_GESTOR_REDE_H
