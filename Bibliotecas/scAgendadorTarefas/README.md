# scAgendadorTarefas

## Responsabilidade no Ecossistema
A biblioteca `scAgendadorTarefas` atua como o "Motor de Millis" ou um Scheduler Cooperativo leve, garantindo o paralelismo ilusório do sistema.

## Papel no Projeto
- Centraliza a matemática exaustiva da função `millis()`.
- Evita a criação de dezenas de variáveis `unsigned long ultimoTempo` no arquivo `.ino` principal.
- Provê uma sintaxe extremamente limpa: `agendador.adicionarTarefa(lerSensor, 2000)` para rodar a cada 2 segundos.
- Mantém o super loop limpo e legível. Essencial para o Módulo 2, que precisa calcular consumo, piscar o OLED, monitorar rede e ler entradas sem usar a função destrutiva `delay()`.
