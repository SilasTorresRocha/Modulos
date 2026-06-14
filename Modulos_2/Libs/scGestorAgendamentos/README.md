# scGestorAgendamentos (Módulo 2)

## Responsabilidade Central
O `scGestorAgendamentos` confere a autonomia de nível militar (Redundância offline) ao Módulo 2. Ele garante que os relés continuem executando as automações programadas para dias da semana exatos, **mesmo que o roteador da casa exploda, o Hub suma e a internet caia**.

## Princípios Adotados

### 1. Lista Encadeada/Array Estático Persistente
Esta classe não se apoia em nuvem. Ela gerencia internamente um array rígido de "Jobs" semanais (Máximo 10, para caber na RAM restrita do ESP-12F).
Ao ligar a placa, o construtor busca na `scArmazenamentoLocal` o array salvo.

### 2. O Loop Vigia
Dentro de seu próprio método `loop()`, chamado a cada N ms, ela:
- Obtém o minuto absoluto da semana usando ponteiro do `scRelogioSincronizado`.
- Varre o array de `Agendamento`s ativos para aquele Dia da Semana específico.
- Se houver match (Ex: 08h:00m na Terça), ele dispara o Relé X via ponteiro da `scGestorReles`.

### 3. Exposição de CRUD para a UI
O `scControladorMenu` pode chamar métodos como:
- `adicionarAgendamento(Relé 1, SEG, 08, 00, LIGAR)`
- `listarAgendamentos()`
- `excluirAgendamento(id)`

Todo *Update* na lista engatilha imediatamente um *Commit* na flash, garantindo que se a energia cair no milissegundo seguinte, o timer foi salvo com sucesso.

## Dependências
- `scRelogioSincronizado`
- `scArmazenamentoLocal`
- `scGestorReles` (Ponteiro)
- `scLogger`
