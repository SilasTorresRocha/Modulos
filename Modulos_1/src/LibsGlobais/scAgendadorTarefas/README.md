# scAgendadorTarefas

## Responsabilidade no Ecossistema
A biblioteca `scAgendadorTarefas` atua como o "Motor de Millis" ou um Scheduler Cooperativo leve, garantindo o paralelismo ilusório do sistema.

## Papel no Projeto
- Centraliza a matemática exaustiva da função `millis()`.
- Evita a criação de dezenas de variáveis `unsigned long ultimoTempo` no arquivo `.ino` principal.
- Provê uma sintaxe extremamente limpa: `agendador.adicionarTarefa(lerSensor, 2000)` para rodar a cada 2 segundos.
- Mantém o loop limpo e legível. Essencial para o Módulo 2, que precisa calcular consumo, piscar o OLED, monitorar rede e ler entradas sem usar a função destrutiva `delay()`.

## Arquitetura e Decisões de Design

### Retorno de ID e Dinamismo (`alterarIntervalo`)
Diferente de Schedulers simples que retorna apenas um `booleano`, a função `adicionarTarefa()` devolve o `ID (índice)` numérico da tarefa criada. Isso permite controle individual em tempo real, como acelerar a leitura de um Encoder rotativo quando o usuário entra num menu, ou desligar a atualização do display quando a tela entra em *Idle/Sleep*, sem precisar alocar novas tarefas na memória.

### Prevenção do (Resume Seguro)
Quando uma tarefa de longa duração é pausada por muito tempo e depois é retomada, temporizadores disparam a função instantaneamente repetidas vezes, pois a matemática acusaria que ela está "muito atrasada". O `scAgendadorTarefas` previne esse *tiro de canhão* na inicialização: ao receber um comando de *Resume* no método `setEstadoTarefa()`, ele recria a âncora de tempo local (`ultimoTempo = millis()`), obrigando a tarefa a recomeçar seu ciclo limpo a partir daquele exato momento.

### Máquina do Tempo (`forcarProximaExecucao`)
Quando é necessário forçar um disparo fora de hora (ex: usuário solicitou leitura forçada via MQTT), usar loops travados seria letal. A biblioteca permite disparar qualquer tarefa no próximo loop apenas recuando sua âncora de tempo no passado: `ultimoTempo = millis() - intervalo`. Isso atende à condição de ativação de forma limpa, barata em CPU e sem quebrar a estrutura do loop.

### Agendamento Relativo (Anti-Inundação de Processamento)
No *Agendamento Absoluto* (`ultimoTempo += intervalo`), se a placa engasgar levemente (ex: falha na reconexão de Wi-Fi), as tarefas tentarão disparar freneticamente em sequência para "compensar" os ciclos perdidos, sobrecarregando o processador com uma inundação indesejada. 
No **Agendamento Relativo** adotado por esta biblioteca (`ultimoTempo = tempoAtual`), a âncora de tempo é cravada **antes** da execução, sempre baseada no *agora*. Se a placa engasgar, a tarefa é rodada normalmente apenas *uma vez* ao retornar, e só disparará de novo após o intervalo regular se passar novamente. Essa é uma forma segura e interessante para Telas (UI), acionamento de relés e leitura de sensores.

### Dupla Blindagem Anti-Overflow 
O clássico bug de estouro do `millis()` que paralisa o Arduino aos 49 dias é blindado na raiz pela lógica de diferença temporal entre variáveis `uint32_t` *unsigned*. Ainda que o Ecossistema `sc` atue com reinicialização diária preventiva às 03:00 da manhã (anulando as chances de chegar perto dos 49 dias), essa matemática age como redundância vital ("o seguro morreu de velho").
