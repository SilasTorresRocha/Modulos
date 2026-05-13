#include <scMQTTLib.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

// --- CONFIGURAÇÕES DE REDE ---
const char *ssid = "NOME_DA_SUA_REDE";
const char *password = "SENHA_DA_SUA_REDE";

// --- INSTÂNCIA DA BIBLIOTECA ---

// OPÇÃO 1 (Padrão): Utiliza o servidor padrão da biblioteca (apisilas.ddns.net)
scMQTTLib mqtt_sc("seu_usuario_API", "sua_senha_API");

// OPÇÃO 2 (Avançado): Para utilizar um broker MQTT customizado (Mosquitto
// local, AWS, etc) Descomente a linha abaixo e comente a OPÇÃO 1 para usar um
// IP/Host diferente. scMQTTLib mqtt_sc("seu_usuario_API", "sua_senha_API",
// "NOME_OU_IP");

// Variáveis para simular envio periódico de dados do sensor
unsigned long ultimoEnvio = 0;
const unsigned long INTERVALO_ENVIO = 10000; // Envia a cada 10 segundos
int contador_loop = 0;

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println("\n[SETUP] Iniciando Exemplo Avançado scMQTTLib...");

  // Conectar ao WiFi
  Serial.print("[WIFI] Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[WIFI] Conectado com sucesso!");

  // Iniciar a biblioteca scMQTTLib
  // Ele usa o MAC Address e as credenciais enviadas para criar os topicos
  // automaticamente
  mqtt_sc.iniciar();
}

void loop() {
  // Obrigatório: Mantém a conexão MQTT ativa e recebe as mensagens internamente
  mqtt_sc.manterConexao();

  // ---> RECEBENDO COMANDOS
  // A biblioteca gerencia as mensagens e as disponibiliza de forma fácil
  // através de polling
  if (mqtt_sc.temComando()) {
    String comando = mqtt_sc.obterComando();

    Serial.println("\n------------------------------------");
    Serial.print("[MQTT] NOVO COMANDO RECEBIDO: ");
    Serial.println(comando);

    // Tratamento simples de comandos recebidos
    if (comando == "LIGAR_LED") {
      Serial.println("-> Ação: Ligando atuador (Ex: Rele/Led)...");
      // digitalWrite(LED_BUILTIN, HIGH);

    } else if (comando == "DESLIGAR_LED") {
      Serial.println("-> Ação: Desligando atuador...");
      // digitalWrite(LED_BUILTIN, LOW);

    } else {
      Serial.println("-> Ação: Comando não reconhecido neste sistema.");
    }
    Serial.println("------------------------------------\n");
  }

  // ---> ENVIANDO DADOS (Vários Formatos)
  if (millis() - ultimoEnvio > INTERVALO_ENVIO) {
    Serial.println("[MQTT] Publicando fila de pacotes...");

    // -> Enviando um valor Inteiro (int)
    if (mqtt_sc.enviar("contador", contador_loop)) {
      Serial.println(" - [OK] Inteiro enviado: " + String(contador_loop));
    }

    // -> Enviando um valor Decimal (float) com 2 casas processadas nativamente
    float temperaturaSimulada = 25.50 + (random(-10, 10) / 10.0);
    if (mqtt_sc.enviar("temperatura", temperaturaSimulada)) {
      Serial.println(" - [OK] Float enviado: " + String(temperaturaSimulada));
    }

    // -> Enviando um Texto (String)
    String statusSistema = (contador_loop % 2 == 0) ? "NORMAL" : "AVISO";
    if (mqtt_sc.enviar("status", statusSistema)) {
      Serial.println(" - [OK] String enviada: " + statusSistema);
    }

    // -> Enviando um JSON puramente customizado pelo usuário (cenários
    // complexos com múltiplos campos de uma vez) Atenção para não quebrar a
    // formatação JSON montada manualmente nas strings
    String aqrJSON =
        "{\"humidade\": 60, \"luz_ambiente\": 450, \"modo_noturno\": false}";
    if (mqtt_sc.enviarJSON(aqrJSON)) {
      Serial.println(" - [OK] JSON customizado enviado via string bruta.");
    }

    contador_loop++;
    ultimoEnvio = millis();
    Serial.println();
  }
}
