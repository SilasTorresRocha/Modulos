# scGestorReleM1 (Módulo 1)

## Responsabilidade Central
Atuador complementar e sistema de contingência local que controla um pino atrelado a um Relé de Estado Sólido (SSR) embarcado no próprio Módulo 1.

## Princípios Adotados

### 1. Anti-SPOF Radical (Redundância Descentralizada)
Embora a arquitetura obrigue que o Módulo 1 grite via rede ESP-NOW e MQTT para o Módulo 2 fechar válvulas em casos de vazamento, redes de rádio são suscetíveis a interferências pesadas. O Módulo 1 conta com este canal de relé fixado *localmente* no chip.

### 2. Autonomia de Emergência
Se o `scMonitorGasM1` reportar violação do teto, a `scGestorReleM1` instantaneamente ligará (ou desligará dependendo do acionamento NF/NA) a carga local atrelada, como um mini-exaustor imediato de segurança conectado à bancada. Essa ação é puramente a nível de microcontrolador, não precisando ir ao roteador nem à nuvem para acontecer.

### 3. Fail-Fast
O comportamento local ocorre na mesma fração de tempo de processamento em que a interrupção/alerta foi gerada. Segue estritamente os padrões de não-bloqueio adotados no Ecossistema.

## Dependências
- `scMonitorGasM1` (Monitor que servirá como gatilho disparador da carga).
