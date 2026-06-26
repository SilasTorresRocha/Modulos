# scMonitorSaudeM2 (Módulo 2)

## Responsabilidade Central
Atuando como o "médico residente" da placa, esta biblioteca realiza o monitoramento do hardware físico específico do Módulo 2 (Relés, RTC, ESP-12F e Display).

## Princípios Adotados

### 1. Tolerância a Falhas e Diagnóstico
Se o usuário derramar água na bancada e fechar curto no I2C do display (pinos 4 e 5), o loop do C++ comumente trava infinitamente ao tentar comunicar com a tela.
Para evitar isso, a saúde do barramento é monitorada. Se a `scMonitorSaudeM2` detectar que o display "sumiu", ela manda a `scGestorDisplay` e a `scControladorMenu` desligarem suas atualizações gráficas. Isso salva o loop de engasgar e permite que os relés continuem atuando 100% via rede(Wi-fi/mqtt) e/offline(ESP-NOW Hub) sem que a placa congele.

### 2. Auto-Cura (Watchdog e Correções)
Ela não apenas acusa, mas tenta reverter.
- Se o sensor avisar que o `scRelogioSincronizado` perdeu sincronia de forma bizarra (Epoch foi para ano 1970 do nada), ela flagra o erro para o Backend enviar um novo comando "sincronizar_relogio", impedindo os Relés de ligarem em horários falsos.
- Registra `falha_display` ou `memoria_corrompida` no array `err[]` global que sobe na Telemetria.

## Dependências
- `scSaudeHardware` (Base)
- `Wire` (I2C) para dar pings nos escravos físicos.
