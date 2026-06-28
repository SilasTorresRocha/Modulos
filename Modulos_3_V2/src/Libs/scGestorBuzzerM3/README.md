# scGestorBuzzerM3 (Arauto de Eventos)

## Responsabilidade
Atuar como uma classe "Casca" local que instancia e consome a biblioteca global `scAvisosSonoros`. Gerencia o acionamento do Buzzer passivo no Módulo 3, adicionando regras específicas de UI (bips do LVGL) e acionando sirenes de emergência da casa, sem reinventar a roda base.

## Princípio (Comunicação Acústica Unificada)
Seguindo as regras do ecossistema, os feedbacks e avisos sonoros não podem travar o processador com `delay()`. Especialmente com o motor LVGL rodando em Loop, o Buzzer precisa apitar de forma 100% assíncrona (usando `millis()` ou `micros()` em FSM).

## Funcionalidades
- **Mini-alertas:** Bips curtos e rápidos para transição de estado da rede ( Wi-Fi para ESP-NOW) ou feedback tátil de clique nos botões da Tela Touch.
- **Sons de Navegação:** Acionamentos configuráveis ao abrir menus.
- **Alerta Grave (Sirene):** Disparado em caso de Vazamento de Gás no Módulo 1, módulo crítico inativo no Radar, ou falha geral, tocando intermitentemente até ser reconhecido (Acknowledged) fisicamente ou via nuvem.
- **Mute Mode:** Capacidade de ser silenciado pelas configurações globais via `scDespachanteComandosM3`.
