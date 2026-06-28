# scGestorTelasM3 (A Lógica de Interface e Menus)

## Responsabilidade
É o diretor da orquestra visual. Ele decide "o que" deve aparecer na tela em resposta aos eventos do sistema. É a camada de regra de negócio UI acima da `scMotorGraficoM3`.

## Princípio (Separação de Preocupações - SoC)
O Gestor de Telas não sabe configurar GPIOs de SPI e não deve possuir matemática de alocação de memória gráfica. Ele simplesmente consome as APIs providas pelo `scMotorGraficoM3` para construir a experiência do usuário (UX).

## Funcionalidades e Contratos
- **As 4 Telas Base:**
    - **Tela 1: DESLIGADO (Standby Inteligente):** Backlight desligado, mas o LVGL monitora emergências para "acordar" em caso de gás ou falha.
    - **Tela 2: Descanso de Tela (Slides):** Carrossel lendo imagens `.bin` direto do SD Card, sem decodificação JPG/PNG (Zero peso na CPU).
    - **Tela 3: Dashboard Completo:** Exibe Intensidade do Sinal Wi-Fi dinâmico, SD, MQTT, Hora. Mostra Cards com dados de todos os nós (M1, M2, M4).
    - **Tela 4: Dashboard Simplificado:** Relógio gigante e ícone de "Casa Segura".
- **Menu de Configurações (A Mão Dupla com a Nuvem):**
    - **Globais:** Scan Wi-Fi (testa antes de fazer broadcast `update_wifi`), MQTT (Usuário/Senha testados), Fuso Horário/NTP.
    - **Hub (M3):** Rotação da Tela (0, 90, 180, 270), Brilho PWM, Volume/Mute, Seleção da Tela Idle (Padrão após X segundos).
    - **Módulos Filhos:**
        - Espelho M1: Calibrar Gás, Tempos de alarme (Preparo/Esquecimento).
        - Espelho M2: Adicionar/Remover Agendamentos (Timers), Regras Pós-Queda (ON/OFF) e Reset de Consumo Kw/h.
        - Espelho M4: Inclusão de Digitais e Exclusão.
        - Renomear Nós: Gestão local de "Apelidos" para os MACs, sincronizando as strings amigáveis com o Backend.
- **Abas de Diagnóstico e Radar:**
    - **Status da Malha:** Utiliza as cores da `scRadarEcossistemaM3`:
        - 🟢 **Verde (Online):** Ping < 60s (Wi-Fi saudável).
        - 🟡 **Amarelo (Fallback):** Ping > 60s, MAS chegou por ESP-NOW (Nó ativo, mas isolado da net).
        - 🔴 **Vermelho (Inativo):** Ping > 300s (Desligado/Falhou).
    - **INF (System Info):** Mostra IP do Hub, RAM Livre e histórico de Uptime.
- **Gerenciador de Interrupções Visuais (Overlays):**
    - Pop-Ups vermelhas exclusivas para avisos urgentes vindos da malha (Vazamento, Módulo inativo), invocando a Sirene do `scGestorBuzzerM3`.
- **Acoplamento Inverso:** Eventos físicos tocados na tela geram pacotes JSON que o `scDespachanteComandosM3` validará, ou que o `scRoteadorBridgeM3` repassará à nuvem.
