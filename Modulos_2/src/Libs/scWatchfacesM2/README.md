# scWatchfacesM2 (Módulo 2)

## Responsabilidade Central
A `scWatchfacesM2` nasceu sob o Princípio 5 (*Abstração de Interface Local*). Ela atua puramente no contexto Visual da View (MVC) e foca estritamente nas telas "Idle" (Tempo ocioso) e painéis ricos.

## Princípios Adotados

### 1. Separação de Preocupações Estritas (SoC)
Desenhar relógios analógicos exige uso de cálculos trigonométricos de Seno/Cosseno, e desenhar barras de consumo exige lógica de layout 2D densa. Deixar isso dentro da `scControladorMenu` transformaria o arquivo num monstro inmanutenível. Aqui, tudo é isolado.

### 2. Padronização Global
As *Watchfaces* não inventam comunicação com I2C cru; elas preparam os "Draw Commands" usando os métodos da biblioteca global da casa (`scGestorDisplay`).

### 3. Telas Específicas Contidas
Esta biblioteca fornecerá os layouts prontos para as seguintes chamadas do controlador:
- `desenharRelogioGrande()`: Para quando a prioridade do usuário for visualizar o tempo local atualizado via NTP.
- `desenharDashboardConsumo()`: Apresenta uma HUD com o total KWh acumulado e se os relés estão armados.
- `desenharStatusInf()`: Usado para exibir a tela de diagnóstico, que deve mostrar explicitamente:
  - MAC Address (vital para a comunicação interna via ESP-NOW e identificação).
  - Status da Rede (Online, Offline via ESP-NOW, ou Desconectado).
  - Endereço IP (se Online).
  - Temperatura do Módulo 1 (Sempre exibida, seja via Wi-Fi ou ESP-NOW. Só é ocultada se o Módulo 1 for dado como **Inativo**/Morto).

## Dependências
- `scGestorDisplay`
- `scRelogioSincronizado` (Apenas para usar a hora formatada).
