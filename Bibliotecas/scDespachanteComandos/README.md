# scDespachanteComandos

## Responsabilidade no Ecossistema
A biblioteca `scDespachanteComandos` é o roteador universal (O Despachante). Ela obedece ao Princípio 2 (Comunicação via Contratos Estritos JSON).

## Papel no Projeto
- Desserializa a string JSON recebida da `scMQTTLib` ou via `ESP-NOW` garantindo total segurança de memória através de Alocação na Stack (RAII).
- Atua como Porteiro: Valida o campo `mac_destino` para garantir que pacotes de *Broadcast* do ESP-NOW não acionem módulos errados.
- Verifica se o comando é de Manutenção/Universal do Ecossistema (ex: `atualizar_firmware`, `reiniciar_dispositivo`, `sincronizar_relogio`) e os resolve internamente coordenando as outras bibliotecas auxiliares de forma autônoma.
- Caso seja um comando de Regra de Negócio específico do Módulo (ex: `set_rele`), repassa apenas os argumentos do comando (`JsonVariant args`) para uma função de *Callback* registrada no arquivo principal (`.ino`).

## Arquitetura e Decisões de Design

### Injeção de Dependências Desacopladas
Para não cometer o pecado de usar `Serial.print()` espalhado pelo código (quebrando a padronização e escalabilidade), o Despachante recebe o ponteiro do `scLogger`. Qualquer falha de leitura do JSON é reportada corretamente ao sistema central de log. Ele também recebe o `scConfigOTA` e `scRelogioSincronizado` para orquestrar Comandos Universais sem que o módulo principal saiba que algo aconteceu.

### Validação MAC (Blindagem contra Broadcast)
No ESP-NOW, é comum o Hub transmitir comandos genéricos (`mac_destino: "ALL"`) ou específicos. O Despachante processa o campo `mac_destino` rigorosamente pelo Contrato e destrói o pacote silenciosamente caso o MAC destino não seja a palavra explícita `"ALL"` nem corresponda ao MAC físico exato da placa receptora.

### Proteção de Memória em Alto Nível (RAII)
O uso da `ArduinoJson` é feito com a instância `JsonDocument doc` estrita ao escopo da função `processarPayload`. Isso significa que a RAM emprestada da Stack para desserializar o documento é limpa no momento exato em que a função termina, garantindo 0% de vazamento de memória (Memory Leak) ou Fragmentação ao longo de meses de uso.

### O Callback de Negócios
Quando o Despachante identifica que o comando é `set_rele` (que pertence à placa local e não à infraestrutura universal), ele não joga a String crua para o Módulo lidar. Ele chama o `CallbackComandoLocal(cmd, args)`, entregando a chave primária já validada e um objeto inteligente (`JsonVariant`) contendo os argumentos.
Isso mantém o `loop()` do arquivo `.ino` imaculado, abstrato e focado unicamente nos atuadores elétricos, sem saber como a internet funciona.

### Repasse Híbrido de Comandos Universais (Exceções)
Embora a maioria dos comandos de infraestrutura seja resolvida de forma 100% autônoma pelo Despachante (como `atualizar_firmware` ou `reiniciar_dispositivo`), existem comandos universais que exigem o conhecimento da aplicação principal (o arquivo `.ino`):
- **`solicitar_status` (Heartbeat Instantâneo):** O Despachante não possui acesso aos sensores físicos, logo, não sabe montar a telemetria. Por isso, ele **obrigatoriamente** repassa o comando via `_callbackLocal` para que o módulo force o disparo dos dados através da implementação da telemetria (ex: `scTelemetriaBase`).
- **`set_peer_mac` (Provisionamento P2P):** O Despachante cumpre seu papel de infraestrutura salvando a chave na memória *flash* via `scArmazenamentoLocal`, mas **também** repassa o comando via `_callbackLocal` para que o módulo carregue o MAC do parceiro na memória RAM imediatamente. Isso assegura que a comunicação Mesh (ex: um alerta de vazamento de gás) funcione no mesmo instante, sem necessidade de reinicialização.
