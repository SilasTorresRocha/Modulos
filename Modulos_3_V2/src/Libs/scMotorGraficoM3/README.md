# scMotorGraficoM3 (Renderizador / Driver Visual)

## Responsabilidade
Configurar fisicamente a tela TFT (ILI9488 3.5"), lidar com as instâncias básicas da UI (Botões, Barras, Cores) e chamar periodicamente a engrenagem principal da interface gráfica (LVGL). A leitura do TouchScreen via driver XPT2046 (SPI) também reside nesta camada de hardware.

## Princípio (Isolamento de Barramento SPI)
Na V1 com FreeRTOS, a contenção de Mutex no barramento SPI gerou a falha do código. Na V2 (Bare-Metal), o Motor Gráfico é estritamente orquestrado de modo atômico pelo Loop.
- É o único lugar do firmware que tem permissão para usar as primitivas SPI voltadas para a Tela TFT.
- Os chamados à `lv_task_handler()` e a injeção do TouchScreen (`lv_indev_t`) são feitos de modo calculado para NUNCA encavalar ciclos com a gravação de emergência do SD Card (`scGestorSDCardM3`).
- **Desempenho (DMA e SRAM):** Usa `TFT_eSPI` sem depender de DMA 16-bits nativo do ILI9488 (que sofre bloqueios no S2). Utiliza `tft.pushColors` jogando frames calculados rapidamente da SRAM interna para a tela.

## Contratos e Setup Físico
- Provê funções para o ajuste de brilho da tela (usando GPIO em modo PWM para o pino de backlight).
- Permite controle dinâmico da rotação do driver TFT (0º, 90º, 180º, 270º) sem a necessidade de reiniciar a placa.
- **Driver FS do LVGL:** Tem a responsabilidade de inicializar a interface de File System (FS) do LVGL, fazendo a ponte direta e avisando ao LVGL que o hardware base de leitura é a biblioteca `scGestorSDCardM3`. Isso garante que a leitura dos arquivos `.bin` nativos flua perfeitamente.
- Provê as funções primitivas para quem quiser desenhar algo (criarTela, preencherArea ...).
- Expõe ponteiros controláveis para gerenciar o Input Touch.
- Não possui lógica de navegação. Quem decide "Qual tela deve aparecer hoje" é a lib `scGestorTelasM3`.


Carregar "S:logo.bin" do SD direto pra tela