# scMonitorGasM1 (Módulo 1)

## Responsabilidade Central
Filtro de sinal, análise e disparo de alerta de vazamento de gás. Trata-se de uma classe essencial para a segurança de vida provida por este módulo.

## Princípios Adotados

### 1. Filtro de Ruído (Média Móvel)
Como o rádio Wi-Fi do ESP8266 gera flutuações e picos elétricos (ruído de antena) no conversor ADC, a leitura no pino A0 deve passar por uma amostragem contínua (janela de 10 leituras rápidas) e usar uma média móvel, evitando disparos em falso (falsos positivos).

### 2. Gestão de Limites e Thresholds
Gerencia e adota o valor contratual `sens_gas`, obtido das configurações do Backend ou Hub. Este é o limite dinâmico atual no qual o alarme de gás será acionado.

### 3. Disparo Redundante de Alertas
Assim que o filtro validar um vazamento:
1. Ergue a flag interna `vazamentoDetectado`.
2. Informa aos componentes para tocarem alarmes, displays e ativar relés SSR locais.
3. Informa o despachante / Main `(.ino)` para atirar o alerta P2P (ESP-NOW) direto contra o Módulo 2, além de atualizar o MQTT com a central. Garante o cumprimento da premissa Anti-SPOF.
