# scDespachanteComandosM3 (O Cérebro Executor)

## Responsabilidade
Processar, validar e executar fisicamente todos os comandos e configurações (payloads JSON) que são destinados ao próprio Módulo 3.

## Princípio (Separação de Tráfego e Execução)
Enquanto a biblioteca `scRoteadorBridgeM3` funciona como um carteiro (lendo o CEP/MAC e passando a carta para o lado certo sem abrir o conteúdo íntimo), o `scDespachanteComandosM3` funciona como a gerência. Se a carta é endereçada ao Módulo 3, é o Despachante quem rasga o envelope, lê os argumentos e mexe nas chaves locais.

## Contratos
- Recebe do Roteador os pacotes JSON parseados cujo `mac_destino` seja compatível com o do Módulo 3 ou a tag "ALL".
- Lida com **Comandos Universais** do Ecossistema:
    - Atualização de SSID/Senha Wi-Fi e MQTT (Recebidos da Tela ou da Nuvem).
    - Definição de "Apelidos" para rebatizar os módulos na UI.
    - Sincronização de Relógio Offline (Aviso para ler o RTC Físico e espalhar).
    - Provisionamento de Endereços P2P.
    - Reinício do microcontrolador (Soft Reset).
- Lida com as **Configurações Físicas do M3**: Ajuste de volume local, tempo de Idle da tela, etc.
- Repassa ordens para outras libs internas (tipo: mandar a lib `scGestorBuzzerM3` entrar em Mute Mode, ou dizer à lib `scConfigOTA` para salvar novos tokens na memória flash).
