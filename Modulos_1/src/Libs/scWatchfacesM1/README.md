# scWatchfacesM1 (Módulo 1)

## Responsabilidade Central
Motor Gráfico puramente passivo e informativo. Como o Módulo 1 não tem input do usuário local (botões ou encoders limitados), sua tela é dedicada integralmente à visualização.

## Princípios Adotados

### 1. Modos de Tela (Telas Padrão)
O renderizador deve respeitar a variável `tela_idle` (configurada pelo Backend/HUB ) que contém os 3 modos:
- **1 (Desligado)**: Display totalmente apagado.
- **2 (Completo)**: Tela densa informando a Temperatura do Forno, Nível Analógico do Gás, Horário (via `scRelogioSincronizado`) e os ícones de integridade de conectividade.
- **3 (Essencial)**: Layout simplificado exibindo as grandezas macro.

### 2. Wake-up Inteligente (Override Crítico)
Se o display estiver configurado para ficar em repouso absoluto (Modo 1), esta classe tem a inteligência de monitorar os sensores (via classes de termometria e gás).
Se "O forno ligar" ou se "Houver um vazamento", a classe ignorará temporariamente o preset e acenderá a tela com força total mostrando a natureza da emergência. 

## Dependências
- `scGestorDisplay` (Biblioteca base na nuvem / Hub) e  U8g2. Caso não tenha rede Wifi quem passar a ser o mestre da casa e o hub (Modulo 3) enviando via esp-now . 