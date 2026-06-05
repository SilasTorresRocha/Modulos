# scTelemetriaBase

## Responsabilidade no Ecossistema
A `scTelemetriaBase` constrói o pacote de telemetria padronizado (o "Heartbeat") que toda placa deve enviar, garantindo o "MAC Address como Chave Primária" (Princípio 4).

## Papel no Projeto
- Gera o JSON inicial de status preenchendo as variáveis de infraestrutura: `mac_origem`, `status` ("online"), nível de sinal Wi-Fi (`rssi`), e tempo de estabilidade (`upt_est`).
- Fornece métodos `.adicionar...()` para que o código principal do Módulo anexe os seus dados específicos (ex: estado de relés, valores de sensores) sem precisar reconstruir o pacote JSON do zero.
- Envia o pacote compactado e formatado diretamente via `scMQTTLib`.

## Mecanismos Avançados (Segurança e Desempenho)

### 1. Fim da Fragmentação do Heap (Zero ArduinoJson)
O ecossistema demanda que a telemetria seja enviada ininterruptamente (ex: a cada 5 segundos). O uso contínuo de bibliotecas como `ArduinoJson` para alocar e destruir mapas dinâmicos causaria buracos na memória (Heap Fragmentation), especialmente fatal no ESP8266.
A biblioteca soluciona isso montando o JSON fisicamente em uma String simples. No método `inicializar()`, foi utilizado `_jsonAtual.reserve(300);` e `_erros.reserve(5);` para "lotiar" as ruas na memória de uma vez só. Toda concatenação posterior apenas recicla esse espaço fixo, blindando o microcontrolador contra travamentos de memória em longo prazo.

### 2. O Escapamento do Parser
Como o JSON é construído manualmente, **nunca utilize textos que contenham aspas duplas (`"`)** na função `adicionarString()`. Como os contratos de mensagem são controlados, basta enviar texto limpo (ex: "porta_aberta").

### 3. Proteção Contra Overflow de Rádio (A Regra dos 250 Bytes)
O pacote máximo suportado fisicamente pelas antenas via protocolo `ESP-NOW`(v1) é de **250 bytes**. Se acidentalmente criar um payload maior que isso, a camada de rádio falha silenciosamente ou corrompe a pilha, causando crashes misteriosos.
- **Auditoria Interna**: Antes do envio, a biblioteca verifica se `length() > 250`.
- **Prevenção**: Se exceder, o pacote original é destruído imediatamente.
- **Sinalizador SOS**: Em vez de morrer calada, a placa monta na hora um mini-pacote de emergência com o erro `"OVERFLOW_250_BYTES"` e despacha via MQTT. Assim, o Backend sabe que a placa está viva, mas o último firmware subido inflou os contratos de mensagem de maneira insustentável.