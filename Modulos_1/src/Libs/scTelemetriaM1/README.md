# scTelemetriaM1 (Módulo 1)

## Responsabilidade Central
O empacotador oficial das variáveis vitais deste sensor remoto. Construído estritamente seguindo o documento de `CONTRATOS_MENSAGENS.md`.

## Princípios Adotados

### 1. Aderência Absoluta ao Contrato JSON
Visita ciclicamente as variáveis da estrutura do software, e monta o payload compatível com os limites do pacote ESP-NOW (Max 250 bytes).
Ele engloba os atributos obrigatórios e abreviados: `"temp", "gas", "tx_temp", "t_forno", "alm_prep", "alm_crit"`.

### 2. Recrutamento de Logs (Erros)
Visita instâncias da `scMonitorSaudeM1` para resgatar qualquer falha de barramento 1-Wire, I2C ou problemas com o rádio de telemetria, injetando os erros formatados em strings dentro do array `"err"`.
Após montado, efetua o despacho usando os canais adequados (MQTT / ESP-NOW).

## Dependências
- `scTelemetriaBase`
- `ArduinoJson`
