# scBotaoMultifuncao

## Responsabilidade no Ecossistema
A biblioteca `scBotaoMultifuncao` fornece um tratamento robusto de *debounce* e múltiplos eventos para botões físicos presentes nos módulos, eliminando código espaguete de controle de estado.

## Papel no Projeto
- Monitora os botões (ex: botão de recovery ou acionamento local) de forma completamente baseada em interrupções ou polling temporizado com `millis()`.
- Traduz ruídos elétricos em eventos precisos.
- Dispara *callbacks* automáticos para três situações: 
    1. **Clique Curto:** Para acionamento comum.
    2. **Clique Duplo:** Para alternar abas de display ou funções secundárias.
    3. **Clique Longo (Hold):** Para acionar manobras de emergência como Hard Reset, AP Mode, ou reset de credenciais Wi-Fi (interagindo com a `scConfigOTA`).

