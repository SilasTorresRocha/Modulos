#ifndef SCMQTTLIB_H
#define SCMQTTLIB_H

#include <Arduino.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "scMQTTLib suporta apenas ESP8266 ou ESP32"
#endif

#include <PubSubClient.h>

class scMQTTLib {
private:
  const char *_usuario;
  const char *_senha;
  const char *_servidor;
  uint16_t _porta;

  WiFiClient _clienteWiFi;
  PubSubClient _clienteMQTT;

  String _idCliente;
  String _topicoTelemetria;
  String _topicoComandos;

  // Controle de Rate Limiting (100 msgs / min)
  unsigned long _inicioJanelaLimitarTaxa;
  int _contagemMensagens;
  static const int _LIMITE_MENSAGENS = 100;
  static const unsigned long _TEMPO_JANELA_MS = 60000;

  String _ultimoComando;
  bool _novoComandoDisponivel;

  // Funcoes internas
  String obterMacHardware();
  bool verificarLimiteTaxa();
  void conectarMQTT();

  // Callback estatico necessario para o PubSubClient
  static scMQTTLib *_instanciaAtual;
  static void callbackInternoMqtt(char *topic, byte *payload,
                                  unsigned int length);

public:
  // Assinatura 1: Só pede usuário e senha (assume que o server é o padrão)
  scMQTTLib(const char *usuario, const char *senha);
  // Assinatura 2: Pede usuário, senha e o IP do servidor customizado
  scMQTTLib(const char *usuario, const char *senha, const char *servidor);

  // Inicia e carrega topicos/IDs
  void iniciar();

  // Lógica de Reconexão Automática (Watchdog/Keep-Alive)
  void manterConexao();

  bool temComando();
  String obterComando();

  // Envia JSON puro e validado
  bool enviarJSON(String jsonString);

  // Sobrecargas (enviar) - As funcoes empacotam de forma limpa {"chave": valor}
  bool enviar(String chave, float valor);
  bool enviar(String chave, int valor);
  bool enviar(String chave, String valor);
};

#endif // SCMQTTLIB_H
