# scAvisosSonoros

## Responsabilidade no Ecossistema
A biblioteca `scAvisosSonoros` gerencia os *buzzers* ou sinalizadores sonoros dos módulos de forma puramente baseada em estados e `millis()`.

## Papel no Projeto
- Remove a necessidade do uso de `delay()` para criar sons de aviso.
- O código do Módulo principal apenas chama a intenção desejada (ex: `avisos.tocar(ALARME_TEMPORIZADOR)`).
- Diferencia claramente avisos de emergência de avisos cotidianos:
  - **SIRENE_EMERGENCIA**: Som contínuo e estridente, usado apenas para falhas críticas (ex: Vazamento de Gás, Pane Térmica).
  - **ALARME_TEMPORIZADOR**: Som rítmico (padrão de despertador de telefone ou timer de forno), usado para agendamentos e lembretes normais.
- A biblioteca avalia periodicamente no `loop()` os tempos lógicos e altera os pinos digitais no background.
- Preserva a alta taxa de resposta (leitura de encoders, chaves) do Módulo, que seria destruída por temporizadores síncronos.
