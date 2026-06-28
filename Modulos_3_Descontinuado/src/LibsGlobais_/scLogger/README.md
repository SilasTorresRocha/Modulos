# scLogger

## Responsabilidade no Ecossistema
A biblioteca `scLogger` padroniza o rastreamento de eventos, avisos e falhas críticas, eliminando o uso indiscriminado e bloqueante do `Serial.print()`.

## Papel no Projeto
- Define níveis de log: `INFO`, `WARNING` e `ERROR`.
- Quando em modo de desenvolvimento (Debug), direciona o output formatado (com timestamp do `scRelogioSincronizado`) para a porta Serial.
- Quando em produção, armazenar de forma silenciosa os últimos erros (Crash logs) na memória através da `scArmazenamentoLocal`, ou despachá-los como tópicos especiais via `scMQTTLib`.
- Fundamental para diagnóstico pós-falha de módulos instalados em campo.
