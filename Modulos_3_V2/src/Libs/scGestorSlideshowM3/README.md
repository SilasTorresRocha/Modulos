# scGestorSlideshowM3 (O Motor de Descanso de Tela)

## Responsabilidade
Gerenciar autonomamente a apresentação cíclica de imagens armazenadas na pasta `/Imagens` do SD Card sem onerar o processador.

## Princípio (A Leitura Crua)
Em arquiteturas Bare-Metal single-core como o ESP32-S2, processar a descompressão de dezenas de matrizes de um arquivo JPEG trava o processador por centenas de milissegundos. Se durante este travamento chegar um pacote de vazamento de gás via ESP-NOW, as interrupções de barramento causarão colapso geral.
A "Leitura Burra" (Agnóstica a Compressão) extirpa a necessidade de algoritmos de descompressão.

## Contratos
- **Acoplamento Nativo LVGL:** O gestor varre arquivos com formato nativo de matriz de bytes (`.bin` gerados pelo LVGL Image Converter) do SD Card.
- Executa como uma rotina não-bloqueante no Super Loop. Quando o timer de transição estourar, a biblioteca simplesmente aponta a nova rota do arquivo na interface `lv_img_set_src()` do LVGL.
- O driver gráfico do LVGL lê a memória RAM/SPI do SD e joga direto pra tela (Zero gasto da CPU em cálculos trigonométricos ou huffman).
- Desarma automaticamente mediante atividade touch, liberando o retorno instantâneo do Dashboard Inicial provido pela `scGestorTelasM3`.
