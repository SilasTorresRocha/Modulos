# scControladorMenu (Módulo 2)

## Responsabilidade Central
O `scControladorMenu` atua como o **Controller** na arquitetura MVC do ecossistema. Ele não atua em hardware crítico nem sabe comunicar com a internet; sua única função é criar a ponte inteligente entre a abstração das **Entradas Físicas** (`scEncoderRotativo` e `scBotaoMultifuncao`) e as **Saídas Visuais** (`scGestorDisplay` e `scWatchfacesM2`).

## Princípios Adotados

### 1. Máquina de Estados da Interface
Para evitar "Telas Sujas", a biblioteca implementa uma máquina de estado fechada:
- `ESTADO_WATCHFACE` (Idle - Sem interação por > X segundos).
- `ESTADO_MENU` (Navegação pelas listas).
- `ESTADO_EDICAO` (Quando clica para alterar um valor, ex: mudando o estado Pós-Queda de 'Sempre Ligado' para 'Sempre Desligado').

### 2. Árvore de Menus Profundos
O Módulo 2 é a segunda interface mais rica (atrás do Hub). A biblioteca aloca em Arrays const (PROGMEM, para economizar RAM) a árvore de menus.
- **Watchfaces** -> Relógio / Dashboard / Simples / Desligado.
- **Relé 1 & 2** -> Configurar Gás / Configurações de Consumo.
- **Agendamentos** -> Segunda, Terça...
- **Sistema (INF)** -> Abre abas com IP, MAC.

### 3. Responsividade Total com ISRs
Como o encoder é lido por Interrupções de Hardware (ISR) implementadas pela `scEncoderRotativo`, o Controlador não precisa rodar cálculos longos de *debounce*. Ele apenas consome os deltas do contador. Se a thread de desenho do OLED (`scGestorDisplay`) gastar 50ms para renderizar os pixels, a leitura física do giro do botão pelo usuário não é perdida, provendo um feeling "Premium" de 60fps sem engasgos.

### 4. Inteligência Contextual (Segurança)
Seguindo as rígidas regras do Ecossistema, a comunicação local nunca morre se a internet cair. O menu **ocultará** as opções de "Ações de Gás" APENAS se o Módulo 1 (Gás) estiver **Inativo** (morto/desligado). Se ele estiver apenas "Offline" (sem internet, mas vivo no ESP-NOW da casa), as opções e alertas continuam operando normalmente. O `scControladorMenu` obedece cegamente a essas três flags de estado (Online, Offline, Inativo).

## Dependências
- `scEncoderRotativo`
- `scBotaoMultifuncao`
- `scGestorDisplay`
- `scWatchfacesM2`
- `scGestorReles` (Apenas ponteiro para enviar ordens, nunca chamando `digitalWrite` direto).
