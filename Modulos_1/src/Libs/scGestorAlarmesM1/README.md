# scGestorAlarmesM1 (Módulo 1)

## Responsabilidade Central
Atua como um temporizador passivo e gestor de alertas lógicos e sonoros baseando-se no estado termal e de gás do Módulo 1.

## Princípios Adotados

### 1. Vigilante do Forno
Ao receber da `scLeitorTermicoM1` a indicação de que o forno foi ligado, começa a contabilizar o uptime.
Supervisiona passivamente as variáveis remotas configuradas: `alm_prep` e `alm_crit`.
- Se o tempo ligado ultrapassar o Alarme de "Preparo": Solicita à `scAvisosSonoros` a emissão de bipes suaves (Bip) lembrando a retirada do prato.
- Se ultrapassar o Alarme "Crítico" (Esquecimento): Dispara via `scAvisosSonoros` um alarme sonoro severo de sirene de emergência.

### 2. Vigilante de Gás
Escuta os estados e os gatilhos da `scMonitorGasM1`. Se um vazamento for atestado, a classe imediatamente eleva o nível de atuação do Buzzer para uma sirene de emergência grave, sobrepondo-se aos outros toques.

## Dependências
- `scAvisosSonoros` (Emissão sonora física do hardware via bibliotecas Globais)
- Integração indireta ou instâncias da classe Térmica e de Gás.
