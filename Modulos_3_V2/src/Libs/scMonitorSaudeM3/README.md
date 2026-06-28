# scMonitorSaudeM3 (O Cão de Guarda Local)

## Responsabilidade
Monitorar de perto todos os sinais vitais exclusivos do hardware do Módulo 3 (ESP32-S2).

## Princípio (Fail-Fast)
Falhar Rápido e de forma visível. O sistema jamais deve falhar silenciosamente ou congelar. Esta biblioteca verifica constantemente a integridade do sistema no modo Loop.
- Checa a quantidade de memória RAM (Heap e PSRAM) livre e o maior bloco alocável contíguo.
- Monitora os barramentos I2C (RTC) e SPI (TFT/SD).
- Monitora o tempo de execução do `lv_task_handler()` (Loop gráfico). Se a tela estiver engasgando, levanta flag para ação preventiva.

## Contratos
- Opera com verificações rápidas não-bloqueantes.
- Se detectar um engasgo grave, fragmentação severa de memória, ou travamento no I2C/SPI, a lib deverá acionar alarmes locais (Buzzer/Tela) e levantar flags de segurança (`err`) para serem anexadas na própria telemetria do M3, para que o ecossistema (e a Nuvem) saibam da falha de hardware.
