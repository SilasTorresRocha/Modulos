# scDespachanteComandos

## Responsabilidade no Ecossistema
A biblioteca `scDespachanteComandos` é o roteador universal (O Despachante). Ela obedece ao Princípio 2 (Comunicação via Contratos Estritos JSON).

## Papel no Projeto
- Desserializa a string JSON recebida da `scMQTTLib` utilizando a biblioteca `ArduinoJson`.
- Verifica se o comando é de Manutenção/Universal (ex: `update_wifi`, `reiniciar_dispositivo`, `solicitar_status`) e os resolve internamente, acionando a `scConfigOTA` ou a placa diretamente.
- Caso seja um comando específico do Módulo (ex: `set_rele`), repassa a execução para uma função de *Callback* registrada no arquivo principal do módulo (`.ino`).
- Ajuda a manter a `loop()` da aplicação limpa e abstrata.
