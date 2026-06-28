# scRTCFisicoM3 (O Cronometrista de Emergência)

## Responsabilidade
Gerenciar a comunicação I2C com o módulo RTC DS3231 e efetuar a leitura do sensor de temperatura ambiente embutido no próprio chip.

## Princípio (Redundância Primária de Tempo)
Atua como o **relógio mestre da malha** caso o servidor NTP da `scRelogioSincronizado` (biblioteca global) perca conexão. 
Se a casa ficar sem internet, o Módulo 3 passa a fornecer a hora exata, carimbando os pacotes ESP-NOW (evitando Replay Attacks) e ditando a hora para o M1, M2 e M4 usando o Broadcast via rádio.

## Contratos
- Não possui lógica de rede.
- **Entrada:** Recebe requisições locais (chamadas de função) para informar o tempo.
- **Saída:** Retorna o *timestamp UNIX* (uint32_t) atual e a temperatura (float) do chip.
- **Tratamento de Falha:** Se o barramento I2C travar ou o DS3231 não responder, deve levantar flag de erro sem travar o Loop.
