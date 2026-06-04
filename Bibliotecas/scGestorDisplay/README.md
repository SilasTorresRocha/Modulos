# scGestorDisplay

## Responsabilidade no Ecossistema
A biblioteca `scGestorDisplay` atende ao Princípio 5 do projeto ("Abstração de Interface Local"), controlando a atualização de telas (OLEDs) de forma assíncrona.

## Papel no Projeto
- Focada **exclusivamente no display SSD1306 (OLED I2C)** para evitar inchaço da biblioteca (bloatware). Se outros módulos usarem TFTs complexos, isso será tratado no escopo do próprio módulo.
- Gerencia o loop de desenho do display sem bloquear a execução principal da placa.
- Permite a alternância de Abas/Páginas de forma transparente (ex: mudar da Watch Face principal para a tela de "INF" ou "Diagnóstico").
- O módulo apenas atualiza as variáveis de estado (`gestorDisplay.setTemperatura(25.5)`) e a biblioteca cuida de atualizar fisicamente os pixels do OLED apenas quando necessário para economizar processamento e barramento I2C.
