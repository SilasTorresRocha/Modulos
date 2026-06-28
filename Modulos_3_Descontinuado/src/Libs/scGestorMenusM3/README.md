# scGestorMenusM3 (O Maestro Absoluto da Interface)

## O Que É
A espinha dorsal (Wrapper Superior) de toda a tela TouchScreen. Substitui as abas cruas da antiga `scWatchfacesM3`, provendo navegação elegante e integrando todos os subpainéis em um TabView roteável, amarrando a casa inteira.

## Responsabilidades e Casos de Uso

### 1. Injeção Global e Navegação
- Inicializa a View mestre (`lv_tabview_create`).
- Roteia as telas:
  - Aba 1: Painel Radar (Consome UI do `scRadarEcossistemaM3`).
  - Aba 2: Painel de Controle Remoto (Consome UI do `scPainelComandosM3`).
  - Aba 3: Painel de Configurações da Casa (Consome UI do `scPainelProvisionamentoM3`).
- Injeta logicamente o teclado (`lv_keyboard_create`) nesta camada mestre para que qualquer caixa de texto de qualquer painel-filho possa invocar a digitação sem re-alocar memória desnecessariamente.

### 2. Feedback Tátil (O "Açúcar" de UX)
- Conecta-se diretamente aos eventos globais do LVGL (Focus, Clicked). 
- Ao registrar qualquer interação humana no Hub, envia ordem ao `scAvisosSonoros` para emitir um BIP_CURTO (sonoro).
- Lê o status de conexão para exibir o IP do Hub fixado no canto (muito requisitado para debug de infraestrutura).

### 3. Autoconsciência Visual
- Possui o botão para o usuário entrar nas configurações do próprio M3, como calibrar a precisão de toque ou alterar as Watchfaces gerais. Não é apenas para configurar os outros, o Hub também cuida da própria apresentação.
