# Módulo 2: Controle de Relés e Interface (Encoder)

**Microcontrolador:** ESP-12F (Bare Metal)

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` (ou `Backend/README.md`) para entender as lógicas de NTP, Comunicação (scMQTTLib), Alertas Padronizados (Buzzer) e Fallback de Rede (Wi-Fi e ESP-NOW) que se aplicam aqui.

## Abordagem de Software
**Bare Metal com Interrupções de Hardware (ISRs)**
*   **Por quê:** A leitura de um encoder rotativo precisa ser **instantânea**; se o usuário girar o botão rapidamente e o loop estiver ocupado desenhando a tela OLED, passos serão perdidos e a experiência será péssima.
*   **Lógica de Hardware Interrupts:** O controle do encoder deve ser feito via pinos configurados com `attachInterrupt()`. Quando o pino muda de estado, uma ISR extremamente curta incrementa/decrementa o `contador_encoder`. O `loop()` principal apenas lê esse contador, atualiza a UI (SSD1306) e cuida da `scMQTTLib`.

## Hardware Necessário
*   **ESP-12F**
*   **Módulo OLED I2C 1.3 in**
*   **Encoder Rotativo Integrado** (Para navegação) + Um botão para confirmação de escolha
*   **2 Botões Físicos Extras** (Usados como atalhos para Ligar/Desligar relés diretamente, ou, se pressionados juntos, entrar em modo Silencioso).
*   **Módulo de Relés 3V (2 Canais)**
*   **Buzzer** (Padrões de alertas definidos no Ecossistema)

## Especificidades da Interface (Sistema de Menus)
O Módulo 2 possui o menu de interatividade física sendo o Segundo mais rico do sistema (Ficando atras apenas do Modulo Central). Utilizando Padrão de Projetos (separar UI numa biblioteca isolada), ele permite:
*   **Agendamento Semanal (Persistência Local):** Diferente de simples temporizadores (timers de contagem regressiva), o usuário pode criar regras por dia da semana (ex: *Desligar Canal 1 do relé às 7:30 da manhã toda segunda-feira*).
    *   *Redundância e Armazenamento:* Para evitar dependência e não falhar caso o Hub ou a Internet caiam, os agendamentos **são armazenados fisicamente na memória da placa** (NVS ou LittleFS do ESP8266).
    *   *Limite de Memória:* Por limitações de hardware do chip, o módulo aceitará um limite rígido de agendamentos (ex: Máximo de 10 agendamentos ativos). Se o usuário quiser criar mais, deverá apagar os antigos.
    *   *Descentralização Configural:* Esses agendamentos podem ser criados, cancelados ou visualizados a partir do Encoder Físico (Local), Módulo 3 (Hub) ou Web.
*   **Estado de Retorno Pós-Queda de Energia:** O usuário pode configurar como cada relé deve se comportar quando a energia elétrica retornar:
    1.  *Sempre Ligado*
    2.  *Sempre Desligado*
    3.  *Último Estado* (Memoriza se estava ligado ou desligado antes da queda e restaura o status).
*   **Configuração Sonora:** Ativar/Desativar som do buzzer (Modo Mute ou Som ativo para navegação nos menus).
*   **Escolha de Tela de Idle:** Relógio Grande, GIFs animados, Mostrar Estatísticas da rede, ou Tela Desligada.
*   **Dashboard Informativo Local:** Exibe Uptime, Relés Ativos, e **Consumo Individual e Total Estimado** em KW/h.
    *   *Nota sobre Consumo:* O cálculo matemático requer a **Potência da Carga (Watts)**. O usuário deve configurar a potência dos equipamentos conectados (ex: Exaustor = 300W) usando o Encoder Físico do Módulo ou recebendo a configuração remotamente pelo Backend/Módulo Central. O módulo monitora há quanto tempo exatamente cada relé está ligado na sessão atual e calcula o consumo com base nessa potência. Os valores acumulados de consumo podem ser resetados manualmente ou remotamente. Exibe também a **Temperatura do Módulo 1**.
*   **Aba de Informações do Sistema (INF):** Uma página exclusiva no final do menu para não poluir as "Watch Faces" principais, contendo diagnósticos essenciais como o **MAC Address** da placa, Endereço IP e status de conexão. Isso auxilia o usuário a identificar a placa quando estiver mapeando "Apelidos" no Dashboard do Hub/Web.

## Reação a Eventos Críticos Inter-Módulos
*   **Regra de Gás do Relé:** Nas configurações da web/central/menu, o usuário pode configurar como a carga ligada ao Relé vai reagir em caso de **Vazamento de Gás**. Essa regra dita a decisão do Módulo 2 quando ele receber o grito de alerta do Hub ou o grito de alerta **P2P Direto (Mesh)** do Módulo 1. A responsabilidade da ação é única do Módulo 2.
*   **Três Modos de Ação Reativa:**
    1.  *Independente:* O Módulo 1 acionar não afeta o relé.
    2.  *Bloquear (Perigo):* O relé é impedido de ligar (ou é desligado imediatamente) pois o que está plugado gera faísca elétrica.
    3.  *Obrigatório (Exaustor):* O relé é forçado a ligar para retirar o gás do ar, e se desliga quando o vazamento cessar.
*   **UI Inteligente:** Essas opções reativas só aparecem na telinha do Módulo 2 se o Módulo Central(HUB/Backend) acusar que "O Módulo 1 está Online na casa".
