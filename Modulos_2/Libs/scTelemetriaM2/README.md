# scTelemetriaM2 (Módulo 2)

## Responsabilidade Central
A `scTelemetriaM2` é a empacotadora oficial do módulo. Ela herda ou estende o envelope abstrato fornecido pela biblioteca global `scTelemetriaBase`, e injeta **exclusivamente** os dados da placa dos relés.

## Princípios Adotados

### 1. Obedecer ao Contrato
O arquivo `CONTRATOS_MENSAGENS.md` dita que o Módulo 2 precisa enviar chaves minificadas (devido ao ESP-NOW ter payload estrito de 250 bytes).
Esta biblioteca sabe exatamente o nome da chave (ex: `r1_kw`) e onde ir buscar o dado na memória real (perguntando à `scGestorReles->obterKwhAcumulado(1)`).

### 2. Abstração de Protocolo
A Telemetria não chama o objeto Wi-Fi, MQTT ou ESP-NOW diretamente. Ela simplesmente preenche o `JsonDocument` e o entrega para a biblioteca de transporte `scGestorRede`, mantendo-se fiel ao modelo OSI de camadas.

### 3. Injeção de Erros de Saúde
Antes de fechar o JSON, ela percorre o array de erros críticos levantados pela `scMonitorSaudeM2` (ex: `["display_morto", "ntp_desincronizado", ...]`) e os anexa à chave obrigatória `"err"`, permitindo que o Backend e o Módulo Central saibam que esta placa precisa de reparos.

## Dependências
- `scTelemetriaBase`
- Todos os Componentes da Placa (Para puxar o estado de cada um).
