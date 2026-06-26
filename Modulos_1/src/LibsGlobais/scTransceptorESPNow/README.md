# scTransceptorESPNow

## Responsabilidade no Ecossistema
A biblioteca `scTransceptorESPNow` atua como o Gerenciador de Rádio Mesh de baixo nível, tirando a carga burocrática da `scGestorRede`.

## Papel no Projeto
- Empacota e desempacota pacotes para o protocolo nativo ESP-NOW.
- **Trata o limite estrito de 250 bytes de payload do ESP-NOW:** O ecossistema foi projetado para que NENHUMA mensagem passe desse limite. Se um payload for maior que 250 bytes, a biblioteca **rejeita o envio imediatamente**, pois a fragmentação (esperar partes do pacote) destruiria a performance e bloquearia o microcontrolador.
- Gerencia o *Discovery P2P* dinâmico: Ao receber a ordem de envio, se o MAC de destino não estiver na lista de *peers* cadastrados no rádio, a biblioteca os registra em milissegundos antes de enviar o pacote, suportando redes reativas sem configuração prévia.
- Lida com as chamadas assíncronas de envio e recebimento do rádio através da API C nativa da Espressif, injetando o pacote processado de volta na camada C++ da `scDespachanteComandos`.

## Arquitetura de Plataforma Cruzada (Hardware-Agnostic)
Para respeitar a arquitetura do projeto e suportar chips com capacidades diferentes (Módulo 1/2 com ESP8266 e Módulo 3/4 com ESP32), esta biblioteca utiliza compilação condicional (`#ifdef`).

### O Módulo 4 e a Criptografia Nível Cofre ("Senha da Casa")
Devido às exigências do **Módulo 4 (Biometria)** de não aceitar comandos falsos injetados por dispositivos intrusos no modo de *fallback* offline, a `scTransceptorESPNow` possui a injeção da "Senha da Casa" (`configurarCriptografia(pmk, lmk)`).
- **Em um ESP32 (M3, M4):** As senhas de 16 bytes são injetadas diretamente nos registradores do rádio. Todo pacote entre essas placas passa a ser criptografado fisicamente antes da antena atirar.
- **Em um ESP8266 (M1, M2):** Como a Espressif não implementou suporte transparente a LMK nesses chips primitivos, a biblioteca emite um Aviso de Hardware no `scLogger` e transmite de forma limpa, confiando no filtro MAC da `scDespachanteComandos`.
