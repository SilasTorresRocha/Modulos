# scRoteadorBridgeM3 (O Controlador de Tráfego)

## Responsabilidade
É o motor central de roteamento do Módulo 3, fazendo a ponte direta entre a Borda (ESP-NOW) e a Nuvem (MQTT). 

## Princípio (Alocação Dinâmica Zero)
A falha catastrófica da V1 devido à Heap Fragmentation (causada por instanciar e destruir dezenas de `String`s na interrupção/loop) deve ser suprimida aqui. 
- O Roteador opera estritamente com `StaticJsonDocument` e buffers estáticos (`char buffer[512]`). 
- Proibido o uso de `String`.

## Contratos e Lógica (A Fila de Mensagens)
- **A Armadilha do Callback:** A recepção do ESP-NOW roda em nível de interrupção (ISR). Tentar gravar no SD Card (SPI) diretamente dentro do callback causa colisão de barramento e travamento imediato do Módulo 3.
- **Isolamento via Ring Buffer (Zero Heap):** Para evitar a alocação dinâmica letal de um `std::queue<String>`, a biblioteca implementa um cesto estático e travado na memória: um Ring Buffer circular (ex: `char _filaPacotes[10][256]`).
- **Rotina ISR (Ultrarrápida):** O callback do rádio apenas recebe o pacote bruto, copia o array para a próxima posição do buffer e encerra em 1 microssegundo.
- **Rotina Principal (Bare-Metal):** O Loop invoca a função `processar()` constantemente. Ela verifica se há itens no Ring Buffer, consome a mensagem e então, de forma 100% segura, envia o pacote à Nuvem (MQTT) ou à Caixa Preta (`scGestorSDCardM3`).
- **Se o destino for um Módulo Filho:** Envia para a rede via antena (ESP-NOW ou MQTT dependendo do status de fallback do Módulo filho alvo).
- **Se o destino for "M3" (ou ALL):** Repassa o pacote processado para o `scDespachanteComandosM3` atuar nas engrenagens locais.
