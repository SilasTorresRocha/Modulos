# scRadarEcossistemaM3 (O Olho Que Tudo Vê)

## O Que É
O componente responsável por processar a telemetria bruta recebida pela rede e transformá-la em um Dashboard dinâmico e inteligente, regido estritamente pelos princípios de Escalabilidade Horizontal (N-Módulos).

## Responsabilidades e Casos de Uso

### 1. Rastreamento e Mapeamento Múltiplo (N módulos)
- Seguindo o Princípio 4 da Arquitetura (Escalabilidade Horizontal), não existe limite rígido e não existem nomenclaturas fixas como "Módulo 2 número 1" ou "Módulo 2 número 2". A rede pode conter `N` módulos do mesmo tipo, limitados apenas pela memória física do ESP32-S2.
- O Radar utiliza o **MAC Address** como a única Chave Primária absoluta. Qualquer pacote interceptado de um MAC desconhecido é imediatamente registrado em uma estrutura dinâmica (ex: lista encadeada ou array dinâmico em RAM).
- A interface gráfica LVGL é gerada iterando essa estrutura dinâmica N vezes. O radar cruza a Chave Primária (MAC) com a base de dados da `scArmazenamentoLocal` para desenhar o "Apelido" na tela (ex: "AA:BB..." => "Iluminação Sala").

### 2. Painel de Saúde Visual (As Bolinhas)
- Para cada um dos `N` nós rastreados, aplica a regra matemática de sobrevivência baseada em `millis()`:
  - 🟢 **Verde (Online):** Recebeu pacote via Wi-Fi/MQTT nos últimos 60 segundos.
  - 🟡 **Amarelo (Fallback):** Recebeu pacote via ESP-NOW. Significa que o nó perdeu o roteador da casa e está gritando por rádio direto pro Hub.
  - 🔴 **Vermelho (Inativo):** Mais de 300 segundos em silêncio absoluto.

### 3. Exibição de Telemetria Cruzada
O LVGL deste Radar extrai o suco dos JSONs para a tela principal:
- **Termômetros:** Puxa a temperatura física da própria placa M3 (via sensor interno do DS3231 da `scRTCFisicoM3`) e exibe simultaneamente as Temperaturas recebidas de qualquer módulo M1 na rede.
- **Consumo Energético (Watts):** Processa e soma os KWh e Watts instantâneos recebidos de todos os canais de todos os `N` módulos M2 da casa, apresentando o consumo global ou específico por MAC.
- **Filtros Temporais:** Permite filtrar a visualização no LVGL consolidando dados através do carimbo de tempo da `scRelogioSincronizado`.

## Integração
O Radar não envia comandos. Ele é um **Ouvinte Assíncrono e Dinâmico**. Ele varre a lista de `N` dispositivos vivos interceptados pela `scRoteadorBridgeM3` e desenha os componentes visuais correspondentes no Canvas Gráfico do LVGL.
