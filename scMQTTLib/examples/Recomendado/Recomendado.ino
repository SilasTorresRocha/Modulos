// Faça login em https://apisilas.ddns.net/ e crie seu usuário e senha.
// ATENÇÃO: Instale a biblioteca "ArduinoJson" pelo Gerenciador de Bibliotecas
// da IDE do Arduino. Também é necessário ter a biblioteca "PubSubClient"
// instalada.

#include <ArduinoJson.h> // Importando a biblioteca de JSON para facilitar a criação dos dados
#include <scMQTTLib.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

// CONFIGURAÇÕES DE REDE
#define WIFI_SSID "NOME_DA_SUA_REDE"
#define WIFI_PASSWORD "SENHA_DA_SUA_REDE"

// CREDENCIAIS DA API
// Insira aqui o usuário e senha que você criou no site
#define MQTT_USUARIO "seu_usuario_API"
#define MQTT_SENHA "sua_senha_API"

// CONFIGURAÇÕES DO PROJETO
#define INTERVALO_ENVIO 10000 // Tempo de envio em milissegundos (10000 = 10 segundos)

// Instanciando a biblioteca usando os #defines acima para economizar memória e
// ficar mais organizado
scMQTTLib mqtt_sc(MQTT_USUARIO, MQTT_SENHA);

unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);

  // Configura o LED embutido da placa como saída
  pinMode(LED_BUILTIN, OUTPUT);

  // Conectar ao WiFi
  Serial.print("\nConectando a ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com sucesso!");

  // Iniciar a biblioteca scMQTTLib
  mqtt_sc.iniciar();
}

void loop() {
  // Obrigatório: Mantém a conexão MQTT ativa e recebe as mensagens internamente
  mqtt_sc.manterConexao();

  // RECEBE COMANDOS
  if (mqtt_sc.temComando()) {
    String comando = mqtt_sc.obterComando();

    Serial.println("\n");
    Serial.println("\nCOMANDO RECEBIDO:");
    Serial.println(comando);

    // Tratamento de comandos recebidos para controlar o LED
    if (comando == "ligar") {
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Ação: LED Ligado!");
    } else if (comando == "desligar") {
      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Ação: LED Desligado!");
    } else {
      Serial.println("Ação: Comando não reconhecido para esse dispositivo.");
    }
    Serial.println("\n");
  }

  // ENVIANDO DADOS DE FORMA SEGURA (USANDO JSON)
  if (millis() - ultimoEnvio > INTERVALO_ENVIO) {

    // Cria um documento JSON (Recomendado para evitar erros de formatação no
    // envio) Nota: 'JsonDocument' é para a versão 7+ do ArduinoJson. Se usar a
    // v6, mude para 'StaticJsonDocument<200> doc;'
    JsonDocument doc;

    // Adicione suas variáveis e dados dos sensores aqui:
    doc["temperatura"] = 25.5; // Exemplo de número com vírgula (float)
    doc["umidade"] = 60;       // Exemplo de número inteiro (int)
    doc["status_led"] =
        digitalRead(LED_BUILTIN); // Exemplo lendo o status de um pino
    doc["mensagem"] = "Tudo OK";  // Exemplo de texto (String)

    // Converte o JSON para uma String (texto)
    String pacoteJSON;
    serializeJson(doc, pacoteJSON);

    // Envia o pacote completo para a nuvem
    if (mqtt_sc.enviarJSON(pacoteJSON)) {
      Serial.print("Dados enviados com sucesso: ");
      Serial.println(pacoteJSON);
    }

    ultimoEnvio = millis();
  }
}
