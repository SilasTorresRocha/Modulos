# scMQTTLib

## Responsabilidade no Ecossistema
A biblioteca `scMQTTLib` age estritamente como um túnel ou canal agnóstico de dados (Princípio 3). Ela lida com a conexão com o broker MQTT (ex: Mosquitto) e gere os tópicos.

## Papel no Projeto
- Mantém o heartbeat do cliente MQTT ativo.
- Trata as reconexões com o Broker.
- Faz a subscrição e a recepção de mensagens brutas (String JSON), repassando-as para os tratadores adequados (como o `scDespachanteComandos`).
- Não contém regra de negócio do projeto, apenas gerencia a mensageria pura.
