# Módulo 4: Controle de Acesso (Cofre/Biometria)

**Microcontrolador:** ESP32-C3

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` (ou `Backend/README.md`) para entender as lógicas de NTP, Comunicação (scMQTTLib), Alertas Padronizados (Buzzer) e Fallback de Rede (Wi-Fi e ESP-NOW) que se aplicam aqui.

## Abordagem de Software
**Bare Metal (com uso intensivo de Periféricos Nativos: DMA + SPI + RMT)**
*   **Por quê:** A complexidade não está na lógica de multitarefas, mas sim em evitar que o Microcontrolador trave processando os pixels da tela gráfica e bloqueie a biometria.
*   **Lógica:** O loop principal deve ser leve, coordenando Máquinas de Estado (ocioso, verificando, liberado, negado). O trabalho pesado fica para os barramentos de hardware. O Sensor biométrico processa as imagens por conta própria.

## Hardware Necessário
*   **ESP32-C3**
*   **Leitor Biométrico** (Comunicação via UART).
*   **Tela LCD Circular SPI GC9A01** e **SD Card**.
*   **Servo Motor** da trava física (Gerenciado via sinal PWM). *Nota: Este módulo atua como um sistema de demonstração. Para um projeto real de controle de acesso de alta segurança, o ideal é substituir o Servo Motor por uma **Trava Solenoide** robusta, projetada em padrão "Fail-Secure" (mantém travado em caso de falta de energia).*
*   **Fita de LED WS2812B** (Acionada pela interface RMT do C3 para efeitos rápidos de loading/verde/vermelho sem consumir a CPU no envio dos bits de cor).
*   **Sensor Fim de Curso (Micro-Switch)**: Um pequeno sensor de contato físico colado no curso da lingueta mecânica da trava. Isso servirá para enviar feedback confiável e real sobre o "Status da Porta/Cofre", confirmando via software se a trava mecânica *realmente* engatou ou não (diferenciando-se da fita LED e do pino de comando do Servo, que são atuadores "cegos").
*   **Buzzer** (Padrões de alertas).

## Especificidades do Processamento Gráfico
*   **SPI + DMA (Direct Memory Access):** O compartilhamento do barramento SPI entre o Cartão SD e a Tela Circular precisa, obrigatoriamente, ser regido pelo DMA. Dessa forma, é possível jogar frames de imagens em Slideshow (várias fotos passando em sequência com intervalos de tempo) ou até carregar GIFs dinâmicos do SD direto para o painel LCD, sem o ESP32-C3 travar no `loop`.

## Integração de Gestão (Cadastro/Exclusão)
*   **Descentralização de Configuração:** A placa tem poder para ler as impressões digitais, bater contra sua memória e autorizar o giro do Servo. No entanto, os **comandos administrativos** de gravar um novo usuário (Cadastrar nova Digital) ou Apagar um usuário do banco biométrico, devem vir através da tela do Módulo 3 (Central Touch) ou pelo Backend Web. O Módulo 4 apenas recebe a ordem MQTT de "entre em modo de captura" ou "Delete ID X".
*   **Segurança Estrita de Destravamento Remoto:** Se o usuário acionar um botão na tela do Módulo 3 (Hub) para "Abrir o Cofre", o Módulo 4 deve validar a origem desse comando. Ele só deve aceitar comandos de liberação se chegarem pelo Wi-Fi/MQTT (rede autenticada), ou se chegarem via rádio ESP-NOW com **criptografia nativa ativa** e cujo `mac_origem` seja **exatamente** o MAC Address cadastrado do Hub (Módulo 3). Em um cenário de fallback aberto/desconectado do Hub, o destravamento via rádio deve ser bloqueado sumariamente para evitar adulterações por dispositivos maliciosos na rede local.
