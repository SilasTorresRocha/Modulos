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
*   **Encoder Rotativo Integrado** (Para navegação fluida)
*   **2 Botões Físicos Extras** (Usados como atalhos para Ligar/Desligar relés diretamente, ou, se pressionados juntos, entrar em modo Silencioso).
*   **Módulo de Relés 3V (2 Canais)**
*   **Buzzer** (Padrões de alertas definidos no Ecossistema)

## Especificidades da Interface (Sistema de Menus)
O Módulo 2 possui o menu de interatividade física sendo o Segundo mais rico do sistema (Ficando atras apenas do Modulo Central). Utilizando Padrão de Projetos (separar UI numa biblioteca isolada), ele permite:
*   **Agendadores de Tempo:** Agendar acionamentos (Ligar/Desligar Relé X em horário Y com alta precisão de relógio, baseada no NTP).
*   **Estado de Retorno Pós-Queda de Energia:** O usuário pode configurar como cada relé deve se comportar quando a energia elétrica retornar:
    1.  *Sempre Ligado*
    2.  *Sempre Desligado*
    3.  *Último Estado* (Memoriza se estava ligado ou desligado antes da queda e restaura o status).
*   **Configuração Sonora:** Ativar/Desativar som do buzzer (Modo Mute ou Som ativo para navegação nos menus).
*   **Escolha de Tela de Idle:** Relógio Grande, GIFs animados, Mostrar Estatísticas da rede, ou Tela Desligada.
*   **Dashboard Informativo Local:** Exibe Uptime, Relés Ativos, e **Consumo Individual e Total Estimado** em KW/h.
    *   *Nota sobre Consumo:* O módulo monitora há quanto tempo exatamente cada relé está ligado em sua sessão atual (zerando ao desligar). Os valores acumulados de consumo (Relé 1, Relé 2 e Total) podem ser resetados/redefinidos tanto manualmente pelo menu físico, quanto remotamente pelo Módulo Central ou Backend Web. Exibe também a **Temperatura do Módulo 1** (exemplificando a integração do ecossistema).

## Reação a Eventos Críticos Inter-Módulos
*   **Regra de Gás do Relé:** Nas configurações da web/central/menu, o usuário pode configurar como a carga ligada ao Relé vai reagir em caso de **Vazamento de Gás detectado pelo Módulo 1**.
*   **Três Modos de Ação Reativa:**
    1.  *Independente:* O Módulo 1 acionar não afeta o relé.
    2.  *Bloquear (Perigo):* O relé é impedido de ligar (ou é desligado imediatamente) pois o que está plugado gera faísca elétrica.
    3.  *Obrigatório (Exaustor):* O relé é forçado a ligar para retirar o gás do ar, e se desliga quando o vazamento cessar.
*   **UI Inteligente:** Essas opções reativas só aparecem na telinha do Módulo 2 se o Módulo Central acusar que "O Módulo 1 está Online na casa".
