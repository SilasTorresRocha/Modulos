# scAvisosSonoros

## Responsabilidade no Ecossistema
A biblioteca `scAvisosSonoros` gerencia os *buzzers* ou sinalizadores sonoros dos módulos de forma puramente baseada em estados e `millis()`.

## Papel no Projeto
- Remove a necessidade do uso de `delay()` para criar sons de aviso.
- O código do Módulo principal apenas chama a intenção desejada (ex: `avisos.tocar(ALARME_TEMPORIZADOR)`).
- A biblioteca avalia periodicamente no `loop()` os tempos lógicos e altera os pinos digitais no background usando PWM (frequências).
- Preserva a alta taxa de resposta (leitura de encoders, chaves, MQTT) do loop.

## Arquitetura e Decisões de Design

### Filtro de CPU (Early Return)
Como o buzzer vai passar 99,9% do tempo desligado (`SILENCIO`), o motor do `atualizar()` avalia esse estado na primeira linha e aplica um `return;` imediato. Isso impede que a CPU perca ciclos preciosos calculando as variáveis de `millis()` e cruzando as decisões de *switch/case* inutilmente a cada passagem do loop. Cada picossegundo importa.

### Máquina de Estados
A lógica dos padrões rítmicos (`BIP_DUPLO` e `ALARME_TEMPORIZADOR`) foi implementada sob um semáforo de variável simples (`_passoAtual`). A placa entra na função, vê que ainda não deu o tempo limite daquele passo, sai em 1 microssegundo e vai atualizar a tela OLED ou ler o Encoder. Quando o tempo bate, ela muda o estado do pino, avança o `_passoAtual` e recomeça a contagem. Um sistema não-bloqueante real.

### Sirene Bipolar Efeito Doppler Simulado
Em vez de um zumbido fixo de um Buzzer Ativo, o uso da função `tone()` (PWM para Buzzer Passivo) permite a modulação sônica. Para o padrão `SIRENE_EMERGENCIA`, a máquina alterna abruptamente a cada 400ms entre as frequências de **800Hz (grave)** e **1500Hz (agudo/estridente)**. Isso cria um som de "alarme europeu de ambulância", atraindo a atenção imediata e instintiva para falhas críticas (ex: Vazamento de Gás ou pane no hardware).

### Acústica de Usabilidade
Os bips cotidianos também foram mapeados acusticamente para melhorar a interação do usuário com a máquina: 
- **Navegação**: Sons curtos de navegação batem a `2000Hz` (rápidos e agudos para cliques de encoder/botão).
- **Sucesso/Concluído**: Confirmações (`BIP_LONGO`) utilizam `1200Hz`, criando uma sensação sonora grave de "peso" e estabilidade para indicar que a configuração foi efetivamente salva.
- **Mecânica de Auto-Destruição**: Padrões rítmicos finitos tocam seus passos e, ao terminar, devolvem o próprio motor para o estado de `SILENCIO`. O código `.ino` chama a função apenas uma vez (`tocar()`) e esquece.
