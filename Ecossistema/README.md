u # Ecossistema IoT Modular - Documentação Geral

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

Todos os módulos dependem das seguintes tecnologias como base:

*   **scMQTTLib (Infraestrutura Genérica):** 
    *   A `scMQTTLib` é uma API **independente e de propósito geral**. Ela atua estritamente como um **canal/túnel de comunicação** (abstraindo o MQTT em TX/RX puros, utilizando tópicos para telemetria).
    *   O Backend Web/Servidor se comunica com essa API externa via **Rotas HTTPS + Token** para enviar comandos ou ler telemetria dos ESPs. A biblioteca nunca será customizada com lógicas de negócio específicas deste projeto (ex: pegar temperatura de uma cidade) — qualquer regra de negócio deve vir do Backend que consome e orquestra essa API.
    *   **Exemplo Prático (Comunicação Backend -> API):**
      ```bash
      # Pegar os últimos 10 dados de telemetria
      curl -X GET "https://apisilas.ddns.net/api/v1/telemetria?chave_api=SUA_CHAVE&limite=10" 
      # Enviar um Comando externamente (Backend mandando ligar um relé)
      curl -X POST "https://apisilas.ddns.net/api/v1/comandos/enviar_externo?chave_api=SUA_CHAVE" -d "comando=ON"
      ```
*   **Contratos JSON (Padronização de Dados):**
    *   Para garantir que os módulos conversem entre si perfeitamente sem falhas de conversão, todos os pacotes de dados trafegados na rede MQTT e ESP-NOW obedecem a **Contratos JSON** rígidos (utilizando `ArduinoJson`).
*   **NTP:** Todos os módulos devem obter data e hora. Em caso de falha de internet, o Módulo Central (Módulo 3) atuará como servidor/fornecedor de NTP via rede local ou ESP-NOW para o restante das placas.

## 3. Escalabilidade e Identificação (Gestão de Dispositivos)

A arquitetura do projeto foi desenhada para crescer. É possível ter, por exemplo, múltiplos "Módulos 1" ou "Módulos 2" numa mesma residência (ex: 3 Fogões, 4 Exaustores).

*   **Identificação por MAC Address:** O sistema nunca assume um "ID 1, 2, 3" arbitrário que exigiria um servidor DHCP complexo. A chave primária (ID) de cada módulo no sistema será o seu **MAC Address** físico.
*   **Gestão de Apelidos (Aliases):** Para facilitar a vida do usuário, o Hub (Módulo 3) e o Backend Web exibirão o dispositivo pelo seu apelido (ex: "Relé Cozinha (MAC: AA:BB:CC)"). O usuário selecionará a *entidade física exata* que deseja configurar baseada nesse MAC.
*   **Redundância Visual:** Telas (Watch faces) e painéis dos Módulos (como o Módulo 2) possuirão uma aba de "Informações" para que o usuário verifique facilmente o MAC e o IP da placa que ele está segurando.

## 3. Gestão de Estado de Vida (Ping-Pong)

A saúde de cada placa é validada constantemente (Heartbeat):
*   **`Online`**: Dispositivo conectado ao Wi-Fi e trocando mensagens com o MQTT (Backend / Módulo 3).
*   **`Offline`**: Wi-Fi caiu ou está inacessível. O módulo está enviando dados e se conectando apenas via **ESP-NOW** com o Módulo 3.
*   **`Inativo`**: Quando um módulo (que já esteve conectado antes) para de se comunicar por muito tempo. Se o Módulo 3 (ou o Backend) não receber mensagens dele no tempo previsto + tolerância, o sistema assume que ele foi desligado da tomada ou quebrou.

## 4. Sistema de Fallback e Rede Híbrida (Wi-Fi e ESP-NOW)

A conexão de rede é o pilar do sistema e possui um fallback rigoroso. A topologia de rede física opera no modelo **Híbrido (Hub Ativo, Periféricos Reativos)**:
*   **Hub (Módulo 3 - ESP32-S2):** *Always-On*. Mantém Wi-Fi e ESP-NOW ligados simultaneamente (o ESP32 possui hardware robusto para isso).
*   **Periféricos (Módulos 1, 2, 4):** *On-Demand*. Operam 100% via Wi-Fi/MQTT. O ESP-NOW fica totalmente desligado. Se a conexão cair, eles desligam a antena Wi-Fi e ativam o rádio ESP-NOW. Isso evita que chips como o ESP8266 sofram com perdas de pacotes MQTT e aquecimento tentando fazer malabarismos de interrupção entre dois protocolos de rádio.
1.  **Atualização de Wi-Fi:**
    *   Se o usuário alterar os dados de Wi-Fi pela tela do Módulo 3, este valida se a conexão funciona. Sendo um sucesso, ele atualiza as credenciais via MQTT para o servidor Backend. O Backend então repassa esses dados de SSID/SENHA para as demais placas.
    *   O módulo **NÃO DEVE** apagar a rede atual até testar a rede nova e comprovar que tem internet. Se der falha, a placa volta para a antiga e avisa o servidor que a rede nova é inválida.
2.  **Queda do Wi-Fi (Entrando no Fallback):**
    *   Se um módulo periférico perder o sinal de Wi-Fi, o ícone na tela mostrará "Sem Sinal".
    *   Ele desliga o Wi-Fi normal e ativa o **ESP-NOW** (Tornando-se Reativo) para procurar o Módulo 3.
    *   Como o roteador pode mudar de canal (1 a 13) e o ESP-NOW exige que ambos estejam no mesmo canal, o módulo "órfão" vai **escanear e iterar os canais Wi-Fi** via ESP-NOW até achar o Módulo 3.
    *   Ao encontrar o Módulo 3, ele enviará suas mensagens pendentes e perguntará: "Existem dados novos de Wi-Fi para mim?". Se sim, tenta conectar no novo Wi-Fi.
3.  **Ping de Retorno (Recovery - Saindo do Fallback):**
    *   Como o módulo periférico no modo ESP-NOW está com o Wi-Fi desligado, ele nunca saberá se o roteador da casa voltou a funcionar.
    *   Para resolver isso, a cada X minutos (ex: 5 minutos), o módulo deve parar o rádio ESP-NOW, religar o `WIFI_STA`, e tentar conectar ao roteador. Se falhar, volta rápido para o ESP-NOW. Se conectar, ele manda um pacote MQTT de "Voltei a ficar Online".
4.  **Provisionamento de Endereços (Discovery P2P):**
    *   *Problema:* Como o Módulo 1 (Forno) sabe o endereço MAC do Módulo 2 (Relé) para atirar um alerta de gás P2P direto contra ele, e como eles sabem o MAC do próprio Hub para falar com ele sem internet?
    *   *Solução:* O Hub (Módulo 3) é o orquestrador do sistema. Quando a casa toda liga no Wi-Fi, o Hub envia um comando chamado `set_peer_mac` avisando *"Meu MAC (Hub) é XX:XX...", e também "Ei Módulo 1, o MAC do Módulo 2 é AA:BB:CC..."*. Dessa forma, a rede mesh se autoconfigura de forma dinâmica e todos conhecem os MACs vitais uns dos outros.

> [!WARNING]
> **O Falso Positivo de Internet (Armadilha IoT):**
> A função `WiFi.status() == WL_CONNECTED` informa apenas se a placa está conectada ao roteador, mas não garante saída para a internet. Para evitar que os dados se percam caso a fibra da rua caia, o Fallback opera em dois cenários, graças à função `internetDisponivel()` da `scMQTTLib`:
> *   **Cenário 1 (Sem Rede Local):** Se `WiFi.status() != WL_CONNECTED`, o roteador caiu. A placa ativa o ESP-NOW imediatamente.
> *   **Cenário 2 (Sem Internet):** Se o Wi-Fi está conectado, mas a placa falha em enviar dados MQTT após X tentativas, significa que o roteador não tem saída para a web. A placa também ativa o ESP-NOW para tentar chegar ao Módulo 3 (que pode ter alguma rota alternativa ou simplesmente para manter a casa funcionando offline).

> [!WARNING]
> **Segurança no Fallback e a "Senha da Casa" (Criptografia ESP-NOW):**
> Para não perder a casa se o Hub quebrar e não comprometer a segurança, a arquitetura exige uma "Senha da Casa" (Chave Simétrica):
> 1. **Backend:** O usuário cria uma "Senha Mestra" no Backend, que a converte em exatos 16 bytes.
> 2. **Provisionamento Seguro:** Pelo Wi-Fi seguro, o Backend envia a chave. A `scDespachanteComandos` repassa para a `scArmazenamentoLocal` que a grava fisicamente na placa.
> 3. **Sobrevivência:** Quando o roteador cai, a `scTransceptorESPNow` lê a chave do disco e a injeta como PMK/LMK no rádio.
> 4. **Prevenção de Roubo:** Se o Hub queimar, um novo Hub assume ao receber a mesma senha. Se o Módulo 4 for roubado fisicamente, o Backend simplesmente manda o Hub (M3) revogar e ignorar o MAC da placa roubada, inutilizando-a.
> *Nota: O Módulo 4 (ESP32) exigirá essa criptografia de hardware.*

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
*   Exemplo prático (Rede Saudável): O Módulo 1 (Forno) acusa gás. O Backend ou o Módulo 3 processam esse dado e enviam imediatamente um Comando MQTT para o Módulo 2 (Relés), mandando que ele ligue o ventilador e bloqueie faíscas.
*   **Exemplo prático (Rede Mista com Falha):** Se apenas o Módulo 1 perder o Wi-Fi, o Backend fica "cego" em relação a ele, pois não tem antena de rádio. O Módulo 1 avisa o Módulo 3 via ESP-NOW. O Módulo 3 **obrigatoriamente** traduz isso, repassa para o MQTT na nuvem (função de Gateway), e assim o Backend ou o próprio Hub podem enviar a ordem pro Módulo 2 via Wi-Fi.
*   **Dinamicidade:** Um módulo não mostra opções no menu de algo que não existe. O Módulo 2 só terá no menu as opções "Configuração de Gás" se o Módulo 3 informar que "O Módulo 1 está Online".

## 8. Bibliotecas Core (Reaproveitamento de Código)

Como todos os módulos (1, 2, 3 e 4) compartilham comportamentos base idênticos, o projeto adota a criação de **Bibliotecas Core** (compartilhadas) para evitar reescrever o mesmo código repetidamente e blindar regras complexas. As principais bibliotecas a serem criadas e incluídas em todos os firmwares são:

1.  **`scGestorRede` (O Motor Híbrido):** 
    *   *Função:* Gerencia ativamente a conexão Wi-Fi e automatiza o chaveamento para ESP-NOW.
    *   *Detalhe:* Usa a função `internetDisponivel()` da `scMQTTLib` para atestar conexão real. É ela quem faz o *scan* iterativo de canais (1 a 13) para o nó órfão encontrar o Módulo 3 (Hub) e gerencia os "pings de retorno" silenciosos para reconectar ao roteador da casa quando ele voltar.
2.  **`scConfigOTA` (Credenciais e Persistência):** 
    *   *Função:* Abstrai o salvamento e leitura de configurações na memória não volátil (LittleFS para ESP8266 ou NVS para ESP32).
    *   *Detalhe:* Fica responsável por guardar fisicamente as credenciais novas de Wi-Fi, MQTT e tokens. Ela também implementa e gerencia o servidor OTA em background, garantindo que o módulo sempre consiga receber os binários `.bin` pela rede de forma transparente e segura.
3.  **`scRelogioSincronizado` (Gestão de Tempo Global):** 
    *   *Função:* Fornece a hora exata a qualquer momento através da função `obterHoraUnix()`.
    *   *Detalhe:* Se a placa tem internet, a biblioteca busca e sincroniza a hora via NTP (Network Time Protocol). Se entra em modo offline, ela cessa as buscas na web, aguarda os pacotes ESP-NOW do Hub (Módulo 3, que possui o RTC DS3231 físico) e ajusta o temporizador interno mantendo a precisão para não quebrar os agendamentos.

## 9. Manutenção Autônoma (Reboot Preventivo Diário)

Para garantir a estabilidade absoluta de todos os módulos ao longo de meses ou anos de operação, o ecossistema adota a estratégia de **Reboot Preventivo Diário** gerenciado pela biblioteca `scSaudeHardware`.

### Por que o Reboot Preventivo Diário é a Melhor Solução?
A rotina de reiniciar a placa de madrugada (às 03:00) resolve três dos maiores fantasmas do IoT de uma única vez:
1. **Zera o millis()**: O contador interno de tempo do microcontrolador sofre um *overflow* a cada ~49 dias, o que poderia quebrar lógicas matemáticas. O reboot previne isso.
2. **Cura da Desfragmentação de Memória**: O empacotamento contínuo de pacotes JSON gera buracos na memória RAM (Heap Fragmentation). O reboot devolve o chip ao seu estado de fábrica, liso e rápido, garantindo que ele não sofra engasgos ao longo dos meses.
3. **Simplificação Extrema do OTA**: Ao invés de manter processos pesados escutando atualizações o dia inteiro, a placa simplesmente verifica o servidor de firmware assim que liga. Ao se auto-reiniciar às 03:00, ela garante que receberá as atualizações na hora mais ociosa do dia, poupando código e processamento na `scConfigOTA`.

### O Paradoxo do Uptime e a Flag Efêmera
Para que o Reboot Preventivo não destrua a métrica real de confiabilidade (MTBF - Tempo Médio Entre Falhas), o ecossistema implementa dois tempos distintos através de uma **Flag Efêmera** na `scArmazenamentoLocal`:
- **Uptime da Sessão:** Tempo real desde o último piscar de energia do processador (máximo de 24 horas).
- **Uptime Estável (`upt_est`):** Minutos antes do suicídio programado das 03:00, a placa salva o tempo total na memória. No boot, ela resgata e **apaga** esse tempo. Se houver uma queda de luz na rua à tarde, a placa ligará e não encontrará a flag salva (pois ela foi deletada de madrugada), zerando a contagem.

Isso significa que o Dashboard no Backend receberá o tempo de sobrevivência real do sistema (ex: "Casa operando perfeitamente há 120 dias"), enquanto silenciosamente os módulos limpam a própria RAM toda madrugada.
