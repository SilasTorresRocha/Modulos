# Ecossistema IoT Modular - Documentação Geral

Este documento descreve a arquitetura base, as regras globais e a topologia de comunicação que valem para **todos os módulos** do projeto (Módulos 1, 2, 3 e 4), além de sua interação com o Backend (Nuvem).

## 1. Topologia e Visão Geral

O sistema é desenhado para ter **alta disponibilidade, redundância e independência**.
Existem dois "cérebros" no sistema, que executam essencialmente as mesmas funções de gerência, porém em escopos diferentes:

1.  **Backend Web (Servidor Online/Docker):** É o ponto de controle central pela internet. Ele provê o painel web (com login, dashboard completo, etc.) e distribui comandos para as placas via MQTT.
2.  **Módulo 3 (Central/Dashboard Touch Local):** É o controlador físico presente no local. Ele tem bateria própria (PSU) para lidar com quedas de energia.

> [!IMPORTANT]
> **Tolerância a Falhas:**
> Se o Módulo 3 pifar, a casa não para de funcionar! O sistema opera normalmente, e todo o controle e configuração poderão ser feitos pelo Backend Web Online.

## 2. Padrões Obrigatórios de Comunicação

Todos os módulos dependem das seguintes tecnologias como base, descritas no arquivo `scMQTTLib`:
*   **scMQTTLib:** Para envio e recebimento de pacotes (telemetria e comandos).
*   **NTP:** Todos os módulos devem obter data e hora. Em caso de falha de internet, o Módulo Central (Módulo 3) atuará como servidor/fornecedor de NTP via rede local ou ESP-NOW para o restante das placas.

## 3. Gestão de Estado de Vida (Ping-Pong)

A saúde de cada placa é validada constantemente (Heartbeat):
*   **`Online`**: Dispositivo conectado ao Wi-Fi e trocando mensagens com o MQTT (Backend / Módulo 3).
*   **`Offline`**: Wi-Fi caiu ou está inacessível. O módulo está enviando dados e se conectando apenas via **ESP-NOW** com o Módulo 3.
*   **`Inativo`**: Quando um módulo (que já esteve conectado antes) para de se comunicar por muito tempo. Se o Módulo 3 (ou o Backend) não receber mensagens dele no tempo previsto + tolerância, o sistema assume que ele foi desligado da tomada ou quebrou.

## 4. Sistema de Fallback de Rede (Wi-Fi e ESP-NOW)

A conexão de rede é o pilar do sistema e possui um fallback rigoroso:
1.  **Atualização de Wi-Fi:**
    *   Se o usuário alterar os dados de Wi-Fi pela tela do Módulo 3, este valida se a conexão funciona. Sendo um sucesso, ele atualiza as credenciais via MQTT para o servidor Backend. O Backend então repassa esses dados de SSID/SENHA para as demais placas.
    *   O módulo **NÃO DEVE** apagar a rede atual até testar a rede nova e comprovar que tem internet. Se der falha, a placa volta para a antiga e avisa o servidor que a rede nova é inválida.
2.  **Queda do Wi-Fi:**
    *   Se um módulo perder o sinal de Wi-Fi, o ícone na tela mostrará "Sem Sinal".
    *   Ele ativará a comunicação **ESP-NOW** para procurar o Módulo 3.
    *   Como o roteador pode mudar de canal (1 a 13) e o ESP-NOW exige que ambos estejam no mesmo canal, o módulo "órfão" vai **escanear e iterar os canais Wi-Fi** via ESP-NOW até achar o Módulo 3.
    *   Ao encontrar o Módulo 3, ele enviará suas mensagens pendentes e perguntará: "Existem dados novos de Wi-Fi para mim?". Se sim, tenta conectar no novo Wi-Fi.

> [!WARNING]
> **Segurança no Fallback (Criptografia ESP-NOW):**
> O protocolo ESP-NOW possui suporte à criptografia usando PMK (Primary Master Key) e LMK (Local Master Key) baseada em MAC Address, o que evita a interceptação de dados sensíveis (como comandos de destravamento de cofre).
> *   **Limitação de Hardware:** Apenas chips da família ESP32 (Módulo 3 - ESP32-S2 e Módulo 4 - ESP32-C3) suportam a criptografia no hardware. Eles formarão peers seguros.
> *   **O ESP8266 (Módulos 1 e 2) não suporta LMK/PMK**, portanto sua comunicação via rádio em fallback será aberta. O Módulo 3 (Hub) é perfeitamente capaz de manter redes mistas (peers criptografados e não criptografados simultaneamente), respeitando o limite interno do chip (geralmente até 10 peers criptografados no modo STA).

## 5. Fallback para Falhas Críticas de Hardware

Caso algum controlador perca a comunicação com seus sensores/atuadores internos (e for possível detectar):
*   Dispara imediatamente uma **Flag de Erro Crítico** para a API (Backend) e para o Módulo Central (via Wi-Fi, ou ESP-NOW em caso de falha de rede).
*   A mensagem deve especificar qual hardware exato está apresentando falha (ex: "Falha de leitura no Sensor MQ-2", "Leitor Biométrico não responde na UART").
*   Para itens "mudos" (ex: Fita LED ou Servos comuns), não há como saber, mas componentes como Termistores, I2C, SPI e botões fim-de-curso podem ter seu estado de saúde monitorado.

## 6. Padronização de Alertas Físicos (Buzzer)

Todos os Módulos (exceto aqueles deliberadamente silenciados no menu) possuem um **Buzzer** local para eventos:
*   **Mini-alertas (bips curtos):** Transição de status (ex: perdeu o Wi-Fi e entrou em modo ESP-NOW).
*   **Alertas Temporizados (alarmes sonoros contínuos):** Fim de timer do forno (Módulo 1) ou de agendadores (Módulo 2).
*   **Sons de Navegação:** Navegação nos menus. O usuário pode desativar o som (Mute Mode) pelo menu ou Backend.
*   **Alerta Grave (Siren):** Disparado em vazamentos de gás (Módulo 1) ou módulo inativo no sistema, até ser reconhecido pelo usuário.

## 7. Comunicação Inter-Módulos (Ação Reativa)

Os módulos operam de forma independente, mas seus dados podem acionar rotinas em outros lugares do ecossistema. 
A arquitetura sugere que a comunicação seja intermediada (via Backend ou Módulo 3):
*   Exemplo prático: O Módulo 1 (Forno) acusa gás. O Backend ou o Módulo 3 processam esse dado e enviam imediatamente um Comando MQTT para o Módulo 2 (Relés), mandando que ele ligue o ventilador e bloqueie faíscas.
*   **Dinamicidade:** Um módulo não mostra opções no menu de algo que não existe. O Módulo 2 só terá no menu as opções "Configuração de Gás" se o Módulo 3 informar que "O Módulo 1 está Online".
