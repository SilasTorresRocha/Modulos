# Módulo 1: Monitoramento Crítico (Forno e Gás)

**Microcontrolador:** ESP-12F (Bare Metal)

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` (ou `Backend/README.md`) para entender as lógicas de NTP, Comunicação (scMQTTLib), Alertas Padronizados (Buzzer) e Fallback de Rede (Wi-Fi e ESP-NOW) que se aplicam aqui.

## Abordagem de Software
**Bare Metal (Super Loop Otimizado)**
*   **Por quê:** Este módulo é focado em segurança primária. Ele tem sensores diretos (Analógico para MQ-2 e Termistor) e atuadores diretos. O ESP8266 é mais do que capaz de lidar com a tela OLED I2C de forma paralela. Não use FreeRTOS aqui; o overhead no ESP8266 pode gerar latências.
*   **Lógica:** O loop deve ser mantido enxuto, usando base em temporizadores não-bloqueantes (`millis()`), exatamente como na estrutura da scMQTTLib. Se o gás for detectado, os acionamentos e alertas ocorrem na mesma fração de segundo, sem interrupção de um agendador de tarefas complexo.

## Hardware Necessário
*   **ESP-12F**
*   **Multiplexador Analógico (ex: CD4051)**: Como o ESP8266 só possui 1 pino analógico (A0) e precisamos ler o Gás e a Temperatura simultaneamente para calibrar via web, o multiplexador chaveia o pino A0 para ler múltiplos sensores.
*   **OLED I2C 0.96 in**
*   **Sensor de Gás MQ-2** (Saída Analógica)
*   **Termistor NTC 100K**
*   **Buzzer** (Padrões de alertas definidos no Ecossistema)
*   *(Opcional/Planejado)* Pino acionador de carga (Relé SSR) exclusivo, para ligar um exaustor localmente até o fim do vazamento de gás.

## Especificidades da Interface e Controle
*   **Ausência de Botões Físicos:** Esse sistema não terá sistema de menus por hardware. Toda a calibração de sensores (Sensibilidade do Gás, Tempo de Leitura, Tempo de Alarme do Bolo) será feita remotamente via Backend (Web) ou Módulo 3.
*   **Tela de Idle:** Configurada via Backend/Central. O módulo altera entre 3 modos:
    1.  Desligado: Tela apagada. Liga apenas se o forno for ligado ou se detectar gás.
    2.  Completo: Mostra Temperatura, Nível de Gás e Horário constantemente.
    3.  Essencial: Mostra apenas Temperatura e Gás.

## Temporizadores
*   **Forno:** O usuário pode configurar remotamente um tempo limite no qual o forno deveria ficar ligado (ex: 30 minutos). Fim do tempo gera um disparo do Buzzer e envia notificação via MQTT.

## INFOS
*   **NOTA:** Esses sistema tem 1 tempo para alames e 1 tempo de alarme critipo, um e para alarmer de preparo de alimento e outro para controle de prevenção conta esquecimento de forno ligado. Ampos podem ser configurados remotamente pelo Backend (Web) ou Módulo 3.