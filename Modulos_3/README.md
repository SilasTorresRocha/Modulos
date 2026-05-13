# Módulo 3: Central / Dashboard Touch

**Microcontrolador:** ESP32-S2 (Ou superior)

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` (ou `Backend/README.md`) para entender as lógicas de NTP, Comunicação (scMQTTLib), Alertas Padronizados (Buzzer) e Fallback de Rede (Wi-Fi e ESP-NOW) que se aplicam aqui.

Este módulo atua como o **Centro de Comando Físico** e Broker Local, espelhando as capacidades operacionais do Backend Web.

## Abordagem de Software
**FreeRTOS**
*   **Por quê:** A tela ILI9488 de 3.5" exige alta taxa de transferência de dados via barramento SPI. O uso de uma engine gráfica robusta como a **LVGL** para renderizar botões, abas e animações gera gargalo de processamento.
*   **Lógica Multitarefa:** O sistema devera criar Tarefas Isoladas:
    *   `Tarefa_AtualizarTela`: Exclusiva e de alta prioridade para o LVGL, impedindo que os gráficos travem.
    *   `Tarefa_Network`: Para lidar com a comunicação Wi-Fi, MQTT e chamadas da `scMQTTLib` de forma livre e assíncrona.

## Hardware Necessário
*   **ESP32-S2** (Mais GPIOs disponíveis e alta alocação de memória RAM/PSRAM).
*   **Tela Touch ILI9488 3.5" (SPI 320x480)**.
*   **SD Card SPI** (Sistema de arquivos local para salvar ícones, papéis de parede e cache da interface UI).
*   **Módulo RTC DS3231 (I2C)**: Essencial para manter a data/hora exata do sistema por anos através de sua bateria moeda própria. Ele também possui sensor de temperatura interno, cuja leitura ambiente será mostrada na UI e enviada via MQTT para a Web e para o Módulo 2.
*   **Bateria de Backup (PSU/UPS)**: Um módulo carregador (como o TP4056 + Bateria Li-Ion) para manter o Módulo 3 ativo por algumas horas em caso de queda de energia geral na residência, garantindo a gestão offline (via ESP-NOW) dos sensores da casa que por ventura também tiverem bateria ou para avisar a nuvem da queda de força.
*   **Buzzer** (Padrões de alertas definidos no Ecossistema).

## Recursos de Interface (GUI - Touch)
*   **Dashboard Interativo:** Abas de navegação com botões grandes e touch. Monitora tempo real de todos os módulos conectados na rede: Temperatura (Mód.1), Gás (Mód.1), Status de Relés e Consumo (Mód.2), Porta do Cofre (Mód.4).
*   **Configurador Central (Espelho Total):** Todo e qualquer menu de configuração que exista fisicamente em um módulo filho (ex: menu do Módulo 2) também estará disponível na tela do Módulo 3. Permite configurar o estado de retorno dos relés após queda de energia, redefinir/zerar os valores de consumo de energia dos relés, calibrar sensores, definir temporizadores e gerenciar senhas/biometria.
*   **Configurador de Wi-Fi:** Tela dedicada para adicionar novos SSIDs e Senhas para a casa toda (Valida -> Publica no Backend MQTT -> Backend redistribui para as placas que estão online). Ou via ESP-NOW em casos de falta de internet.

## O Papel de Concentrador Local de Fallback
Quando os módulos periféricos não conseguem acesso à internet, o Módulo 3 assume parte da carga do Backend Web localmente:
1.  Escuta passivamente pacotes via `ESP-NOW` das outras placas.
2.  Mantém a sincronização de tempo para elas (Time Server Local): Se a internet cair, o ESP32-S2 lê a hora exata do chip físico **DS3231** e a distribui pela rede local com total confiança.
3.  Recebe telemetria crítica (ex: alerta de gás) via ESP-NOW e consegue rotear um comando reativo via ESP-NOW para o Módulo 2.
