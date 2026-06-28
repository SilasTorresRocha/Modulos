# scPainelComandosM3 (A Central de Ações e Atuadores)

## O Que É
A interface de intervenção remota. É onde o usuário aperta botões na tela do Hub e a mágica física acontece nos cômodos da casa, independentemente de quantos módulos existam.

## Responsabilidades e Casos de Uso

### 1. Geração Dinâmica de Menus (Baseado em Apelidos)
- A tela não pode ter botões chumbados "Ligar Relé". Ela constrói listas roláveis no LVGL baseadas nos Módulos que estão Vivos (Verdes ou Amarelos no Radar).
- Se houver quatro M2, haverá quatro expansões. Ex: Clicar em "Relé Quarto" abre as opções específicas daquele nó.

### 2. Controle de Relés Múltiplos (M2)
- Exibe o status em tempo real (Switch ON/OFF) do Canal 1 e Canal 2 do `M2_X` específico.
- **Ações Físicas:** Ao arrastar o switch, atira o comando `{"cmd": "set_rele", "args": {"id": 2, "estado": 1}}` direcionado ao MAC do nó selecionado.
- **Agendamentos:** Permite adicionar ou excluir agendamentos físicos complexos (`add_agendamento`) para qualquer canal (ex: "Ligar exaustor às 20h seg/qua/sex").

### 3. Biometria e Segurança de Hardware (M4)
- Envia o comando crítico `abrir_cofre` devidamente assinado com o Timestamp (`ts`) para evitar Replay Attacks (regra rígida do M4).
- Provê interface no Hub para gestão humana do M4: **Cadastrar um novo usuário** (dispara o comando para o M4 entrar no modo gravação do leitor UART) e **Apagar usuários**.

### 4. Calibração e Temporizadores (M1)
- Como o M1 não tem botões, o Hub deve possuir os menus para:
  - Adicionar Timers no Fogão (`alm_prep`).
  - Calibrar e alterar as lógicas do termistor e tolerâncias de alerta (`configurar_operacao`).

## Integração
Usa intensamente a `scDespachanteComandos` para enviar JSONs. Lê os dados de topologia do `scRadarEcossistemaM3` para ocultar funções de nós que estão Inativos, impedindo o usuário de enviar comandos no vazio.
