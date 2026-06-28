# Relatório de Fim de Vida e Backup Arquitetural (Módulo 3 - V1)

Este documento atesta o encerramento do desenvolvimento da primeira versao do Módulo 3. A estrutura deste repositório (`Modulos_3`) **não será apagada**, pois servirá como um inestimável laboratório de estabilização de hardware. Todo o conhecimento extraído duramente do hardware nesta versão será a base para o desenvolvimento do `Modulos_3_v2`.

## 1. Conquistas e Estabilizações de Hardware (O que deu certo)

O desenvolvimento da V1 resolveu os maiores gargalos de integração de bibliotecas do ecossistema. Estes arquivos de configuração devem ser copiados integralmente para a V2:

### 1.1. Motor Gráfico LVGL 8.x (`lv_conf.h`)
Foi alcançada a compilação estável do LVGL 8.x no ambiente do Arduino. O arquivo `lv_conf.h` foi devidamente ativado (`#if 1`), com a profundidade de cores cravada em 16 bits (`#define LV_COLOR_DEPTH 16`), e configurado na pasta correta para ser enxergado pelo compilador (`C:\Users\acers\Documents\Arduino\libraries\lvgl\src`).

### 1.2. Barramento SPI e Display ILI9488 (`User_Setup.h` da TFT_eSPI)
O setup da tela ILI9488 no ESP32-S2 foi lapidado para máxima performance. O barramento SPI foi configurado para **40 MHz**. Mais importante: descobriu-se o bloqueio de hardware onde a tela ILI9488 não suporta DMA nativo de 16-bits. A biblioteca TFT_eSPI teve o DMA desabilitado por software, e o renderizador do LVGL migrou para usar `tft.pushColors` via SRAM, entregando alto FPS de forma segura.

### 1.3. O "Downgrade" (ESP32s2 Board v2.0.17)
Descobri que a versão 3.x do framework ESP32 quebra as assinaturas dos callbacks do rádio (ESP-NOW) e do Watchdog. O ambiente foi estabilizado retornando o pacote de placas para a versão **2.0.17** (baseada no sólido ESP-IDF 4.4).
- O uso de `esp_wifi_set_channel(_canalEspNow, WIFI_SECOND_CHAN_NONE)` foi validado com a inclusão de `<esp_wifi.h>`.

### 1.4. Pinagem Homologada do ESP32-S2 (`PIN.txt`)
O roteamento físico dos pinos foi validado no hardware:
- **I2C:** SDA (8), SCL (9)
- **SPI (Compartilhado TFT + SD):** SCK (36), MISO (37), MOSI (35)
- **SD Card CS:** (10)
- **Buzzer:** (11)

---

## 2. A Autópsia Arquitetural: Por que o código morreu?

A arquitetura de software (o uso de RTOS e ponteiros dinâmicos no roteamento) se mostrou insustentável para a missão crítica de salvar vidas (vazamento de gás) e atuar como Gateway primário. 

### 2.1. O Gargalo do Roteamento (Heap Fragmentation)
Na V1, a biblioteca scRoteadorBridgeM3 utilizava instâncias de objetos String dinâmicos para receber, parsear e repassar pacotes MQTT e ESP-NOW. Em um ecossistema que atua como Gateway centralizando o tráfego de vários módulos, instanciar e destruir dezenas de Strings por segundo em um microcontrolador inevitavelmente causa Heap Fragmentation (fragmentação de memória).

**Regra para V2:** A ponte de rede deve operar estritamente com instâncias de `StaticJsonDocument` fixas (alocar 512 bytes para os JSONs), abolindo o uso de String no tráfego de pacotes.

### 2.2. A Armadilha da Interrupção (O problema do strstr no Rádio)
O callback de recebimento do ESP-NOW (`onDataRecvESPNow`) executa em nível de Interrupção de Hardware (ISR). Na V1, foi cometido o erro de incluir funções bloqueantes e pesadas (como `strstr` para procurar a palavra "gas" no JSON) diretamente dentro da interrupção. Isso sequestra o processador, atrasa o stack do WiFi e causa instabilidade sistêmica, podendo engatilhar falsos positivos se outros pacotes contiverem a mesma string.

**Regra para V2:** Callbacks de ISR devem ser extremamente curtos. A interrupção do rádio deve apenas copiar o payload bruto para um Ring Buffer (Buffer Circular) na memória e levantar uma flag (ex: `novaMensagem = true`). O processamento, o parsing do JSON e a lógica de alarme de gás serão feitos exclusivamente dentro da segurança do fluxo principal do Super Loop.

### 2.3. O Paradigma: FreeRTOS vs Bare-Metal no ESP32-S2

Abaixo está o registro da mudança de paradigma adotada para o futuro `Modulos_3_v2`, fundamentando o abandono do FreeRTOS em favor do Super Loop:

> Quando olhamos para a lista de responsabilidades do Hub (criptografia, ponte de rede, servidor NTP, interface LVGL pesada, rádio, SD Card), o primeiro instinto de qualquer engenheiro de software é jogar um RTOS para orquestrar o caos.
> 
> No entanto, há um detalhe de hardware que muda completamente as regras do jogo: **O ESP32-S2 possui apenas um único núcleo (Single-Core).**
> 
> Aqui está a realidade de tentar rodar tudo isso com o FreeRTOS em um processador de núcleo único, e o motivo pelo qual o bare-metal (Super Loop com Máquinas de Estado) ainda é superior neste cenário específico:
> 
> **1. O Mito do Paralelismo no S2**
> Como o ESP32-S2 tem só um núcleo, o FreeRTOS não executa a rede e a tela "ao mesmo tempo". Ele faz Concorrência, não Paralelismo. O RTOS fica fatiando o tempo: roda um pouquinho da tela, pausa, roda um pouquinho da rede, pausa, roda a criptografia, pausa.
> 
> **2. O Preço Oculto (Context Switch)**
> Para que o FreeRTOS pause uma tarefa e inicie outra, ele precisa fazer o Context Switch (troca de contexto): salvar todos os registradores do processador na memória, trocar os ponteiros de pilha (stack) e carregar os registradores da próxima tarefa. Isso consome ciclos de clock absurdos.
> Em um ambiente de alta demanda de processamento (criptografia, parsing de JSON, renderização de tela), esse gerenciamento constante de tarefas insere micro-atrasos por toda a arquitetura. Em vez de o processador focar em processar os dados do Módulo 1 o mais rápido possível, ele gasta tempo processando qual tarefa deve rodar. Quando o objetivo é velocidade bruta e responsividade sem gargalos invisíveis, delegar o controle de tempo para o sistema operacional adiciona um peso que reduz o desempenho geral do loop.
> 
> **3. A Armadilha do Mutex e o Barramento SPI**
> Este é o problema mais fatal. Como você está usando o barramento SPI compartilhado (SD Card e Tela), se você usar o FreeRTOS, será obrigado a implementar Semáforos (Mutex) para proteger o SPI.
> Se a Tarefa de Rede (Alta Prioridade) receber um pacote e quiser salvar no SD Card, mas a Tarefa da Tela (Baixa Prioridade) estiver no meio da renderização de um botão no barramento SPI, o que acontece? A Tarefa de Rede vai bater no Mutex, ser bloqueada, e o RTOS terá que fazer um downgrade de prioridade ou esperar a tela terminar.
> No fim das contas, as tarefas vão executar de forma sequencial por causa do gargalo do hardware (um SPI só, um núcleo só), mas você estará pagando todo o custo de memória RAM (pilhas separadas para cada tarefa) e custo de CPU (trocas de contexto) do FreeRTOS à toa.
> 
> **4. As Interrupções (ISR) já fazem o trabalho pesado**
> Mesmo em um Super Loop bare-metal, você não vai perder pacotes de rede. O ESP-IDF (a base do Arduino) já roda o rádio WiFi e o ESP-NOW em nível de interrupção de hardware (ISR). Quando um pacote de gás chega, o hardware captura isso no fundo automaticamente e joga em um buffer.
> O seu Super Loop não precisa "escutar" a rede ativamente. Ele só precisa passar no buffer, verificar se há algo lá, processar rapidamente (criptografia/roteamento) e ir para o próximo passo (renderizar LVGL).
> 
> **O Veredito**
> Se você estivesse usando o ESP32 Clássico ou ESP32-S3 (Dual-Core), a minha resposta seria diferente: O RTOS seria obrigatório. Você colocaria toda a rede e rádio no Core 0, e deixaria o LVGL isolado rodando livremente no Core 1. Seria perfeito. Mas no ESP32-S2 (Single-Core), o FreeRTOS atrapalha mais do que ajuda.
> 
> **Como gerenciar tudo isso em Bare-Metal sem virar espaguete?**
> Com Máquinas de Estado Finito (FSM) e Agendadores Não-Bloqueantes. Exatamente como o Módulo 1 funciona.
> - O `loop()` principal roda a milhares de Hertz.
> - A rotina do LVGL (`lv_timer_handler()`) é chamada rapidamente a cada 5ms.
> - O NTP e o Ping da rede usam `millis()` para disparar funções rápidas a cada 1 minuto.
> - A criptografia e roteamento agem apenas como reações instantâneas a flags levantadas pelas interrupções do ESP-NOW.
> 
> O bare-metal exigirá um design mais inteligente e modular (usando Ring Buffers para não perder pacotes entre os ciclos do loop), mas entregará um Hub infinitamente mais rápido, com menor uso de RAM e livre de colisões misteriosas de hardware no SPI.
