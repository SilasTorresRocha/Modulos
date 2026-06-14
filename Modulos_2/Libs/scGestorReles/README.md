# scGestorReles (Módulo 2)

## Responsabilidade Central
A `scGestorReles` é a biblioteca isolada responsável pela **Atuação Elétrica (GPIOs)**, pela **Regra de Gás (Contingência)** e pelo **Cálculo de Consumo (KW/h)**. Ela foi desenvolvida para rodar perfeitamente offline, agindo como um CLP industrial sem necessidade de internet.

## Princípios Adotados

### 1. Desacoplamento Absoluto
Esta biblioteca **não** sabe da existência do Display OLED, dos Botões, do Módulo Central, e nem do Wi-Fi. Ela apenas reage a comandos passados por ponteiro e devolve o estado atual para quem perguntar. Todo e qualquer alerta não é exibido em tela por aqui, e sim logado usando a `scLogger`.

### 2. Estado de Retorno Pós-Queda (Persistência)
Para garantir o Princípio de Redundância e Resiliência, quando o ESP-12F boota, ele obrigatoriamente pergunta à `scArmazenamentoLocal` o que fazer com os relés. O usuário pode ter configurado:
- **Sempre Ligado:** O relé atraca automaticamente no boot.
- **Sempre Desligado:** O relé inicia solto.
- **Último Estado:** A biblioteca salva na flash o status *sempre* que o relé liga ou desliga. No boot, ela restaura o último status conhecido.

### 3. Matemática de Consumo (Sem Internet)
- Ao receber o comando para Ligar (`r1_st = true`), a biblioteca grava a âncora de tempo (Unix Time) através do ponteiro da `scRelogioSincronizado`.
- No ciclo de desligamento (ou via loop periódico de atualização), ela diminui o tempo atual pelo tempo de âncora, descobrindo os segundos decorridos.
- Sabendo a Potência em Watts conectada à carga (salva localmente), o consumo é calculado e acumulado na flash.
- Isso previne que quedas na rede causem perda da medição de consumo daquele dia.

### 4. Gestão e Reset de Consumo (Sincronia Hub/Backend)
- A biblioteca disponibiliza a função `resetarConsumo(id_rele)`. Isso zera os contadores na memória flash.
- **Agendamento de Reset Automático:** A placa atua enviando seu total acumulado. É possível configurar um dia do mês (ex: dia 1) para que o próprio Módulo zere o relógio internamente.
- *(Nota Arquitetural: O ESP-12F manda apenas o valor "Cru/Acumulado" para cima. Filtros robustos de "Consumo por Mês/Dia" ou "Calendário" são processados no Banco de Dados do Backend/Hub, garantindo que o microcontrolador não estoure sua memória RAM calculando históricos passados).*

### 5. Regra de Gás de Emergência
Comunicação de Contingência Severa:
Caso o Módulo 1 envie o aviso P2P de que há **Vazamento de Gás**, a `scGestorReles` entra em modo de exceção `acionarEmergenciaGas()`.
Dependendo do que foi gravado na configuração, ela irá:
- **Ignorar:** (Para cargas seguras, ex: Lâmpadas Blindadas).
- **Bloquear/Desligar:** Desliga o relé instantaneamente para evitar geração de arco elétrico/faísca e bloqueia comandos manuais/agendamentos até a limpeza do ar.
- **Obrigatório (Exaustor):** Força o relé a ligar, sugando o gás da sala até receber o aviso de segurança.

## Dependências
- `scLogger`
- `scArmazenamentoLocal`
- `scRelogioSincronizado`
