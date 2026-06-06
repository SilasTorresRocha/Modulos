# Módulo 1: Monitoramento Crítico (Forno e Gás)

**Microcontrolador:** ESP-12F (Bare Metal)

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` (ou `Backend/README.md`) para entender as lógicas de NTP, Comunicação (scMQTTLib), Alertas Padronizados (Buzzer) e Fallback de Rede (Wi-Fi e ESP-NOW) que se aplicam aqui.

## Abordagem de Software
**Bare Metal (Loop Otimizado)**
*   **Por quê:** Este módulo é focado em segurança primária. Ele tem sensores diretos (Analógico para MQ-2 e Termistor) e atuadores diretos. O ESP8266 é mais do que capaz de lidar com a tela OLED I2C de forma paralela. Não use FreeRTOS aqui; o overhead no ESP8266 pode gerar latências.
*   **Lógica:** O loop deve ser mantido enxuto, usando base em temporizadores não-bloqueantes (`millis()`), exatamente como na estrutura da scMQTTLib. Se o gás for detectado, os acionamentos e alertas ocorrem na mesma fração de segundo, sem interrupção de um agendador de tarefas complexo.

## Hardware Necessário
*   **ESP-12F**
*   **Sensor de Temperatura DS18B20 (Digital 1-Wire)**: Substitui o termistor analógico da ideia original. Como o ESP8266 possui apenas 1 pino analógico (A0), usar um sensor digital num pino digital comum libera o pino A0 de forma *exclusiva e ininterrupta* para a leitura crítica do Sensor de Gás. Isso simplifica o hardware eliminando a necessidade de um Multiplexador e evita atrasos no Loop causados por chaveamento de sinal analógico.
*   **OLED I2C 0.96 in**
*   **Sensor de Gás MQ-2** (Saída Analógica - Conectado exclusivamente no A0)
*   **Buzzer** (Padrões de alertas definidos no Ecossistema)
*   *(Opcional/Planejado)* Pino acionador de carga (Relé SSR) exclusivo, para ligar um exaustor localmente até o fim do vazamento de gás.

## Especificidades da Interface e Controle
*   **Ausência de Botões Físicos:** Esse sistema não terá sistema de menus por hardware. Toda a calibração de sensores (Sensibilidade do Gás, Tempo de Leitura, Tempo de Alarme do Bolo) será feita remotamente via Backend (Web) ou Módulo 3.
*   **Tela de Idle:** Configurada via Backend/Central. O módulo altera entre 3 modos:
    1.  Desligado: Tela apagada. Liga apenas se o forno for ligado ou se detectar gás.
    2.  Completo: Mostra Temperatura, Nível de Gás e Horário constantemente.
    3.  Essencial: Mostra apenas Temperatura e Gás.

## Lógica de Detecção Dupla (Forno Ligado)
Como o Módulo 1 é apenas sensorial e não possui relé cortando a energia do forno, a inferência de que o forno foi ligado (para disparar temporizadores) ocorre por dois métodos simultâneos, garantindo altíssima precisão:
1.  **Derivada Térmica (Taxa de Variação):** Mede a velocidade do aquecimento em Graus/Segundo (°C/s). Se a temperatura subir abruptamente fora de um padrão climático normal, ele deduz o acionamento e liga o timer. *(Nota Técnica: O M1 não possui RTC de hardware, então o cálculo do delta-t usará a função nativa `millis()`. A precisão da derivada pode ter micro-variações dependendo da carga do super-loop, mas para a detecção térmica de um forno, isso é completamente irrelevante e tolerável).*
2.  **Ponto de Ruptura Absoluta:** Como dias muitos frios podem mascarar a taxa de variação inicial, se a temperatura ambiente ultrapassar um limite absoluto insalubre (ex: 55°C), o sistema força o estado de "Forno Ligado", independentemente da taxa de variação.

## Temporizadores
*   **NOTA:** O sistema possui dois alarmes temporizados independentes que podem ser configurados remotamente:
    *   **Alarme de Preparo:** Timer clássico para lembrar de tirar a comida do forno.
    *   **Alarme Crítico (Esquecimento):** Controle de segurança que dispara o Buzzer como sirene e alerta a casa toda se o forno permanecer ligado além do tempo máximo permitido (ex: 2 horas ininterruptas), assumindo que o usuário esqueceu o equipamento ligado.

## Ação Reativa Direta P2P (Anti-SPOF)
Em caso de vazamento de gás crítico, o Módulo 1 enviará seu alerta via MQTT/ESP-NOW para o Hub (Módulo 3). No entanto, para evitar que o Hub se torne um *Ponto Único de Falha (SPOF)* para reações críticas de segurança, o Módulo 1 também atirará um comando **P2P direto** via rádio ESP-NOW usando o MAC Address do Módulo 2 (conhecido via Discovery).
*   *Importante:* O Módulo 1 apenas "Grita que tem gás" para o Módulo 2. Ele **não** ordena se o Módulo 2 vai ligar ou desligar nada. A inteligência e responsabilidade da reação fica a cargo da configuração interna do próprio Módulo 2.

## Versionamento OTA e Controle de Status (Aplica-se a todos os Módulos)
Para garantir que as placas (incluindo o Módulo 1) estejam rodando a versão mais recente do firmware via atualizações OTA, a rede adota um sistema de **Polling via Comando (O Hub/Web Pergunta)** para auditar as versões.
Isso evita o problema clássico de IoT onde a placa envia sua versão apenas no *boot* e o pacote se perde se o broker estiver reiniciando junto.
*   **Comando `solicitar_status`:** Quando o Dashboard Web é aberto, ou em momentos de auditoria, o Backend atira o comando `solicitar_status` para a rede.
*   **Resposta (Callback de Feedback):** Ao receber esse comando, o módulo processa e devolve um pacote assíncrono contendo seu status de hardware e a versão exata do firmware atual (ex: `"versao_fw": "1.0.2"`). Esse mecanismo transfere o controle do fluxo para os "superiores" (Hub/Backend) permitindo checar quem está vivo e atualizado sob demanda.