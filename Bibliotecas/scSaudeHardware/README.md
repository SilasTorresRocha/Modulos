# scSaudeHardware

## Responsabilidade no Ecossistema
A biblioteca `scSaudeHardware` garante a "Tolerância a Falhas" (Princípio 1) em nível de silício, atuando como o sistema imunológico da placa.

## Papel no Projeto
- Aciona e alimenta o *Hardware Watchdog Timer*. Se o processador travar por causa de um loop infinito mal programado no módulo, ou por interferência eletromagnética (ruído de relés), a placa será reiniciada fisicamente após N segundos.
- Coleta métricas vitais da placa: Memória RAM livre (prevenção de vazamento de memória), Temperatura do chip (se suportado pelo ESP) e Uptime (tempo ligado desde o último reset).
- Disponibiliza esses dados para serem injetados no Heartbeat pela `scTelemetriaBase`.
