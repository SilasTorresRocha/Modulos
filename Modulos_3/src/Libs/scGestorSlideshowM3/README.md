# scGestorSlideshowM3 (O Protetor de Tela Inteligente)

## O Que É
A biblioteca visual responsável por garantir que o painel TFT do Módulo 3 se comporte como um produto final refinado, rodando Watchfaces personalizadas e rotinas de leitura gráfica direta via Cartão SD, sem explodir a memória SRAM do chip.

## Responsabilidades e Casos de Uso

### 1. Sistema Anti Burn-in (Leitura Burra Jpeg)
- Monitora a flag global de inatividade (ex: 60 segundos sem toques na tela).
- Paralisa o LVGL temporariamente e acessa a pasta física `/slides/` do Cartão SD (via `scGestorSDCardM3`).
- Utiliza a biblioteca matemática `TJpg_Decoder` para "mastigar" imagens fotográficas `.jpg` de 480x320 pixels. Em vez de subir os 300KB de imagem de uma vez pra RAM, ele processa blocos ("chunks") de 16x16 pixels e joga direto pro hardware do display (`TFT_eSPI`).
- Se houver toque na tela, aborta a decodificação no meio e resgata a tela do LVGL.

### 2. Gestão de Watchfaces Dinâmicas
- O Hub pode carregar watchfaces personalizadas lendo imagens de fundo (`/watchfaces/`) ou elementos gráficos que não caberiam na memória flash limitada do ESP32.
- A biblioteca abstrai a lógica de abrir e fechar ponteiros para a SDLib, impedindo colisões catastróficas de barramento SPI (VSPI) entre o Leitor SD e o Display LCD.
