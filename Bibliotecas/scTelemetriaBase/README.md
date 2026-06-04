# scTelemetriaBase

## Responsabilidade no Ecossistema
A `scTelemetriaBase` constrói o pacote de telemetria padronizado (o "Heartbeat") que toda placa deve enviar, garantindo o "MAC Address como Chave Primária" (Princípio 4).

## Papel no Projeto
- Gera o JSON inicial de status preenchendo as variáveis de infraestrutura: `mac_origem`, `status` ("online"), versão do firmware e nível de sinal Wi-Fi (`rssi`).
- Fornece métodos `.adicionar()` para que o código principal do Módulo anexe os seus dados específicos (ex: estado de relés, valores de sensores) sem precisar reconstruir o pacote JSON do zero.
- Envia o pacote compactado e formatado diretamente via `scMQTTLib`.