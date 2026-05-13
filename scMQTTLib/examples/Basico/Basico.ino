#include <scMQTTLib.h>
// Instanciando a Biblioteca apenas com Usuário e Senha (servidor padrao sera
// usado: apisilas.ddns.net)
scMQTTLib mqtt_sc("usuario_ABCD", "senha123");
void setup() {
  Serial.begin(115200);

  // Voce deve iniciar seu WiFi primeiro antes!
  WiFi.begin("nome_da_rede", "senha_da_rede");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConectado com sucesso!");

  // Inicia efetivamente a geracao de tópicos e preparo do Library
  mqtt_sc.iniciar();
}
void loop() {
  // Essa funcao gerencia reconexoes e o PubSubClient loop (necessita ser
  // chamada na main loop do arduino)
  mqtt_sc.manterConexao();

  if (mqtt_sc.temComando()) {
    String comando = mqtt_sc.obterComando();
    Serial.print("Comando recebido pela biblioteca: ");
    Serial.println(comando);
  }

  // Exemplo de envio a cada 5 segundos
  static unsigned long ref = 0;
  if (millis() - ref > 5000) {
    if (mqtt_sc.enviar("temperatura", 25.50f)) {
      Serial.println("Dado enviado com sucesso!");
    }
    ref = millis();
  }
}