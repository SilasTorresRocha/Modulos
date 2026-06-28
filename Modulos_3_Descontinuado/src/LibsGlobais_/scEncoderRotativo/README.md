# scEncoderRotativo

## Responsabilidade no Ecossistema
A biblioteca `scEncoderRotativo` gerencia a leitura mecânica de alta velocidade de encoders rotativos, garantindo que nenhum passo seja perdido, diferentemente do polling comum do `scBotaoMultifuncao`.

## Papel no Projeto
- Acopla os pinos DT e CLK do encoder a Interrupções de Hardware (ISR) via `attachInterrupt()`.
- Interrompe o processador instantaneamente ao detectar um giro físico, incrementando ou decrementando a contagem de forma segura antes de devolver o controle ao loop.
- Mantém o código principal do módulo protegido contra falhas de leitura ou "pulos" de menu causados por delays inerentes do Wi-Fi ou MQTT.
