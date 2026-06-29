#include "scMQTTLib.h"

// Inicializa o ponteiro estático da instância
scMQTTLib *scMQTTLib::_instanciaAtual = nullptr;

scMQTTLib::scMQTTLib(const char *usuario, const char *senha) {
  _usuario = usuario;
  _senha = senha;
  _servidor = "apisilas.ddns.net"; // Servidor padrao
  _porta = 1883;                   // Porta padrao MQTT
  _ultimoComando = "";
  _novoComandoDisponivel = false;
  _inicioJanelaLimitarTaxa = 0;
  _contagemMensagens = 0;
}

scMQTTLib::scMQTTLib(const char *usuario, const char *senha,
                     const char *servidor) {
  _usuario = usuario;
  _senha = senha;
  _servidor = servidor; // Servidor customizado
  _porta = 1883;
  _ultimoComando = "";
  _novoComandoDisponivel = false;
  _inicioJanelaLimitarTaxa = 0;
  _contagemMensagens = 0;
}

void scMQTTLib::setCredenciais(const char* usuario, const char* senha) {
  _usuarioStr = String(usuario);
  _senhaStr = String(senha);
  _usuario = _usuarioStr.c_str();
  _senha = _senhaStr.c_str();
}

String scMQTTLib::obterMacHardware() {
  String mac = WiFi.macAddress();
  mac.replace(":", "");
  return mac;
}

void scMQTTLib::iniciar() {
  _instanciaAtual = this;

  // Cria o ID baseado no padrao <usuario>_M1_<MAC> para evitar colisao
  _idCliente = String(_usuario) + "_M1_" + obterMacHardware();

  // (ESP Publica e API assina)
  _topicoTelemetria = String("telemetria/") + _usuario;
  // (ESP Assina e API Publica)
  _topicoComandos = String("comandos/") + _usuario;

  // Configura a base do MQTT
  _clienteMQTT.setClient(_clienteWiFi);
  _clienteMQTT.setServer(_servidor, _porta);
  _clienteMQTT.setCallback(scMQTTLib::callbackInternoMqtt);
  _clienteMQTT.setBufferSize(512); // Previne heap nulo na Global Initialization do ESP8266

  _inicioJanelaLimitarTaxa = millis();
}

void scMQTTLib::callbackInternoMqtt(char *topic, byte *payload,
                                    unsigned int length) {
  if (_instanciaAtual) {
    String payloadString = "";
    for (unsigned int i = 0; i < length; i++) {
      payloadString += (char)payload[i];
    }
    _instanciaAtual->_ultimoComando = payloadString;
    _instanciaAtual->_novoComandoDisponivel = true;
  }
}

bool scMQTTLib::temComando() {
  return _novoComandoDisponivel;
}

String scMQTTLib::obterComando() {
  _novoComandoDisponivel = false;
  return _ultimoComando;
}

void scMQTTLib::conectarMQTT() {
  if (!_clienteMQTT.connected()) {
    Serial.print("[scMQTTLib] Conectando ao broker MQTT em: ");
    Serial.println(_servidor);

    if (_clienteMQTT.connect(_idCliente.c_str(), _usuario, _senha)) {
      Serial.println("[scMQTTLib] Sucesso: Conectado ao broker MQTT.");
      // Assinar topico de comandos do usuario
      _clienteMQTT.subscribe(_topicoComandos.c_str());
      Serial.print("[scMQTTLib] Inscrito no topico: ");
      Serial.println(_topicoComandos);
    } else {
      Serial.print(
          "[scMQTTLib] Erro: Falha na conexao! Estado (CONNACK code): ");
      int state = _clienteMQTT.state();
      Serial.println(state);

      // Msg de retorno nativo (state/return)
      if (state == 4 || state == 5 || state == MQTT_CONNECT_BAD_CREDENTIALS ||
          state == MQTT_CONNECT_UNAUTHORIZED) {
        Serial.println("[scMQTTLib] Erro: Usuario ou Senha invalidos!");
      } else if (state == 2 || state == MQTT_CONNECT_BAD_CLIENT_ID) {
        Serial.println("[scMQTTLib] Erro: Client ID rejeitado!");
      } else {
        Serial.println(
            "[scMQTTLib] Erro: Servidor indisponivel ou falha na rede Wifi.");
      }
    }
  }
}

void scMQTTLib::manterConexao() {
  if (WiFi.status() != WL_CONNECTED) {
    // Retorna pois nao e o papel da lib gerenciar a conexao da rede local, mas depende dela
    return;
  }

  if (!_clienteMQTT.connected()) {
    // Delay nao travante para nao floodar
    static unsigned long ultimoAtraso = 0; // Ou mete um -5000 ja para entrar logo de inicio
    if (millis() - ultimoAtraso > 5000) {
      conectarMQTT();
      ultimoAtraso = millis();
    }
  } else {
    // Loop padrao basico do PubSubClient
    _clienteMQTT.loop();
  }
}

bool scMQTTLib::verificarLimiteTaxa() {
  unsigned long agora = millis();

  if (agora - _inicioJanelaLimitarTaxa >= _TEMPO_JANELA_MS) {
    // Reset da janela pois os 60 segundos passaram
    _inicioJanelaLimitarTaxa = agora;
    _contagemMensagens = 0;
  }

  if (_contagemMensagens >= _LIMITE_MENSAGENS) {
    Serial.println(
        "[scMQTTLib] Erro: Rate limite da Lib (100 msgs/min) foi excedido!");
    return false;
  }

  _contagemMensagens++;
  return true;
}

bool scMQTTLib::enviarJSON(String jsonString) {
  if (!_clienteMQTT.connected()) {
    Serial.println("[scMQTTLib] Erro: Nao enviado. MQTT desconectado!");
    return false;
  }

  if (!verificarLimiteTaxa()) {
    return false; // Bloqueado pelo Rate Limit
  }

  // Publica efetivamente usando o topico proprio do usuario
  return _clienteMQTT.publish(_topicoTelemetria.c_str(), jsonString.c_str());
}

bool scMQTTLib::enviar(String chave, float valor) {
  // Empacota {"chave": 25.50} sem bibliotecas pesadas de formato Json
  String json = "{\"" + chave + "\": " + String(valor, 2) + "}";
  return enviarJSON(json);
}

bool scMQTTLib::enviar(String chave, int valor) {
  // Empacota {"chave": 25}
  String json = "{\"" + chave + "\": " + String(valor) + "}";
  return enviarJSON(json);
}

bool scMQTTLib::enviar(String chave, String valor) {
  // Empacota {"chave": "valor"} com escape apropriado
  String json = "{\"" + chave + "\": \"" + valor + "\"}";
  return enviarJSON(json);
}

bool scMQTTLib::internetDisponivel() {
  // Retorna true se a conexão TCP com o servidor na nuvem estiver ativa
  return _clienteMQTT.connected();
}

