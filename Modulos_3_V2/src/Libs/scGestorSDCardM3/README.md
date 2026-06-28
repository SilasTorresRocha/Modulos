# scGestorSDCardM3 (A Caixa Preta )

## Responsabilidade
Operar o barramento SPI **exclusivamente** para operações de leitura e escrita no Cartão SD.

## Princípio (Tolerância a falhas de rede)
No ecossistema, a perda de conexão com a nuvem não pode resultar em perda de dados. Esta biblioteca mantém arquivos em texto plano (JSONL - JSON Lines) no SD Card para evitar problemas de fragmentação de memória (Heap Fragmentation).
- **Crash Logs:** Mantém um histórico diário de erros graves reportados pelos módulos.
- **Fila FIFO Offline:** Mantém a fila de telemetria dos módulos em modo offline, gravando linha a linha.
- **Armazenamento Gráfico (Sistema Inteligente):** 
  - Ao iniciar, verifica se a pasta `/Imagens` existe na raiz do SD.
  - Se não existir, cria a pasta e gera um `LEIA-ME.txt` com o aviso: *"Coloque aqui suas imagens convertidas para .bin através do LVGL Image Converter (Color Format: True color). Arquivos .jpg e .png não funcionarão para poupar o processador."*
  - O motor gráfico (via LVGL) consumirá esses arquivos `.bin` de forma crua, com custo zero de decodificação.

## Contratos
- É totalmente agnóstica à rede (não sabe o que é MQTT nem ESP-NOW).
- Não aloca Strings dinâmicas.
- **Gravação:** Recebe buffers/char arrays (`const char*`) e grava no final do arquivo de forma rápida.
- **Leitura:** Lê a próxima linha disponível e retorna para o despachante responsável subir para a nuvem quando a internet voltar.
- **Conflito de Barramento:** Como divide o SPI com o Display ILI9488, suas chamadas devem ser atômicas e nunca concorrerem durante a renderização gráfica, evitando a colisão SPI fatal (uma das razões para o abandono do RTOS).
