# scGestorRede

## Responsabilidade no Ecossistema
A biblioteca `scGestorRede` é o motor híbrido de conexão do dispositivo. Ela foca na "Tolerância a Falhas" do projeto (Princípio 1).

## Papel no Projeto
- Monitora de forma contínua e não-bloqueante a disponibilidade da rede Wi-Fi e conexão com a internet.
- Faz chaveamento (fallback) inteligente: se o Wi-Fi falhar, ela garante que o módulo possa migrar para operar em modo ESP-NOW.
- Realiza scan de canais e tentativas de reconexão de forma silenciosa para que as funções críticas do módulo não congelem.
- Realiza a volta para rede Wi-fi tambem conforme especificado no Ecosistema.
