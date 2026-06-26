# scGestorRede

## Responsabilidade no Ecossistema
A biblioteca `scGestorRede` é o **Motor Híbrido de Conexão** do dispositivo. Ela foca integralmente na "Tolerância a Falhas" do projeto (Princípio 1) e age como um cão de guarda da comunicação de rádio (Wi-Fi).

## Papel no Projeto
- Monitora de forma contínua e **100% não-bloqueante** a disponibilidade da rede Wi-Fi e conexão com a internet.
- Faz o chaveamento (fallback) inteligente e silencioso: se a internet cair, ela garante que o módulo migre o rádio para operar em modo de "Sobrevivência" (`ESP-NOW`).
- Realiza *scan* de canais (Frequency Hopping) para localizar o Hub (Módulo 3) e tenta o *Reconnect* Wi-Fi a cada 60 segundos, sem congelar as funções críticas da placa.

---

## Arquitetura e Decisões de Design (Mecanismos Avançados)

### 1. A Máquina de 4 Estados (Blindagem contra Race Condition)
Ao invés de assumir que o sistema está online apenas lendo o sinal físico do roteador, a máquina de estados possui a ponte `REDE_VALIDANDO_INTERNET`. 
O fluxo perfeito de sobrevivência age da seguinte forma:
1. Conecta no Roteador (Timeout de 15s).
2. Conectou? A placa dá 10 segundos de Timeout para a `scMQTTLib` realizar o aperto de mão TCP na nuvem.
3. O MQTT confirmou acesso à nuvem? O sistema entra em `REDE_WIFI_CONECTADO` (Saudável).
4. **Qualquer falha nas etapas (Wi-Fi Físico ou Internet Lógica)** derruba a placa para `REDE_FALLBACK_ATIVO` protegendo contra "Roteadores Zumbis".

Para que as bibliotecas não entrem em dependências circulares de cabeçalhos (`#include`), a biblioteca recebe a classe `scMQTTLib` através de *Forward Declaration*.

### 2. Timeouts e Capping Resiliente
- **Timeout de Inicialização (15s):** Se a rede principal não responder em 15 segundos após o boot, a placa desiste para não congelar.
- **Timeout de Validação (10s):** Tempo extra dado para o TCP/MQTT handshake.
- **Tentativas Resilientes (60s):** Estando no modo ESP-NOW, tenta ressuscitar silenciosamente a conexão Wi-Fi a cada 1 minuto.

### 3. O Modo HUB (Âncora)
O Hub (Módulo 3) injeta `configurarComoHub(true)`. Ao fazer isso, se a sua internet cair, ele **nunca** desconecta sua rádio AP e **nunca** escaneia canais. Ele age como a Estrela-Guia, mantendo o ESP-NOW estável para os outros módulos cegos o encontrarem através do *Frequency Hopping*.
