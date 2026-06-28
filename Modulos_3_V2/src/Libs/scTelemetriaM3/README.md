# scTelemetriaM3 (O Empacotador do Hub)

## Responsabilidade
Coletar todos os dados vitais pertinentes exclusivamente ao Módulo 3 (Temperatura ambiente do RTC, RAM livre do Monitor de Saúde, Uptime da sessão, qualidade do sinal Wi-Fi, etc) e formatar o JSON estrito de telemetria.

## Princípio de Padronização
De acordo com os `CONTRATOS_MENSAGENS.md`, todo módulo do ecossistema deve notificar a Nuvem sobre seu status para garantir histórico, manutenção preventiva e dashboards dinâmicos.

## Contratos
- Controlada via `millis()` no agendador principal, executa a cada X segundos de forma rigorosa.
- Solicita os dados às bibliotecas locais (`scRTCFisicoM3.getTemp()`, `scGestorRede.getRSSI()`, `scMonitorSaudeM3.getHeap()`...).
- Gera um payload JSON utilizando `StaticJsonDocument`.
- Entrega a string JSON (minificada) para a infraestrutura de comunicação (provavelmente ao `scRoteadorBridgeM3` para ser despachado para a nuvem via MQTT, ou inserido na fila offline se não houver conexão).
