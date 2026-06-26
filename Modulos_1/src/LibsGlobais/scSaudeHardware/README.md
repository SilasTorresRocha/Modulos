# scSaudeHardware

## Responsabilidade no Ecossistema
A biblioteca `scSaudeHardware` garante a "Tolerância a Falhas" (Princípio 1) em nível de silício, atuando como o sistema imunológico da placa.

## Papel no Projeto
- Aciona e alimenta o *Hardware Watchdog Timer*. Se o processador travar por causa de um loop infinito mal programado no módulo, ou por interferência eletromagnética (ruído de relés), a placa será reiniciada fisicamente após N segundos.
- Coleta métricas vitais da placa: Memória RAM livre (prevenção de vazamento de memória) e Temperatura do chip (se suportado pelo ESP).
- **Gestão de Uptime e Reboot Preventivo:** Reinicia a placa proativamente todos os dias às 03:00 da manhã. Isso previne o estouro da variável global `millis()` (que ocorre a cada ~49 dias) e cura a desfragmentação de memória RAM (Heap) acumulada pelo manuseio contínuo de JSONs.
- **Flag Efêmera (Uptime Estável):** Para não perder a métrica real de MTBF (Tempo Médio Entre Falhas) durante os reboots preventivos, a biblioteca salva na memória não volátil o seu tempo de vida exato minutos antes de se auto-reiniciar. Ao religar, ela resgata esse tempo, soma à nova sessão e apaga a flag. Se houver uma queda real de energia, a flag não existirá no boot, e o Uptime Estável voltará a zero corretamente.
- Disponibiliza a métrica `"upt_est"` para ser injetada no Heartbeat (Telemetria) pela `scTelemetriaBase`.
