# scTransceptorESPNow

## Responsabilidade no Ecossistema
A biblioteca `scTransceptorESPNow` atua como o Gerenciador de Rádio Mesh de baixo nível, tirando a carga burocrática da `scGestorRede`.

## Papel no Projeto
- Empacota e desempacota pacotes para o protocolo nativo ESP-NOW.
- **Trata o limite estrito de 250 bytes de payload do ESP-NOW:** O ecossistema foi projetado para que NENHUMA mensagem passe desse limite. Se um payload for maior que 250 bytes, a biblioteca deve **rejeitar o envio**, pois a fragmentação (esperar partes do pacote) destruiria a performance e bloquearia o microcontrolador.
- Gerencia o *Discovery P2P* (Gestão Dinâmica de MAC Addresses), descobrindo quem é o Hub (Módulo 3) ou como encontrar o Módulo 2 mais próximo na rede Mesh local.
- Lida com as chamadas assíncronas de envio (Sucesso/Falha) e recebimento do rádio, injetando o pacote final processado diretamente na `scDespachanteComandos` da mesma forma que a `scMQTTLib` faria.
