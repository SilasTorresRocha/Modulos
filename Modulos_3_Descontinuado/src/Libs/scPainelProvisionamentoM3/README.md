# scPainelProvisionamentoM3 (O Núcleo de Configuração do Ecossistema)

## O Que É
A biblioteca visual e lógica responsável por gerenciar a entrada de dados do usuário (via teclado virtual LVGL completo) e provisionar toda a casa. O Módulo 3 é o ponto único de configuração; os outros módulos dependem integralmente das configurações propagadas por ele.

## Responsabilidades e Casos de Uso

### 1. Gestão de Teclado Virtual (LVGL)
- Instancia e gerencia um `lv_keyboard` para permitir a digitação complexa de senhas, SSIDs, usuários e nomes, sem depender de celular ou computador.
- Associa automaticamente o teclado aos campos de entrada (`lv_textarea`) quando o usuário toca neles.

### 2. Validação Estrita de Credenciais (Wi-Fi e MQTT)
- Coleta SSID e Senha do Wi-Fi, além de Usuário e Senha do MQTT.
- **Validação Pré-Broadcast:** O Hub **NUNCA** envia uma rede cegamente para os módulos. Ao clicar em salvar, a placa usa a `scGestorRede` para tentar se conectar à rede fornecida. Apenas se o teste de conexão tiver sucesso (e não usar nomes proibidos de laboratório como redes "IOT"), o Hub emite o pacote `update_wifi` e `update_libmqtt` em modo Broadcast (`FF:FF:FF:FF:FF:FF`) para propagar na casa inteira.

### 3. Gestão de Apelidos (Aliases) e Identificação
- Ninguém quer olhar para o MAC `"00:10:FA:6E:38:4A"`. O Painel de Provisionamento possui uma interface de "Renomear Dispositivos".
- O usuário seleciona um MAC detectado na rede e vincula um apelido humano (ex: `M2_3` vira "Relé do Quarto").
- Esses "Aliases" são salvos no NVS/LittleFS do Hub (`scArmazenamentoLocal`) e injetados nas telas dinâmicas do LVGL, para que todos os menus subsequentes utilizem o nome amigável.

### 4. Configuração de Telas (Watchfaces Globais)
- O provisionamento também gerencia a "Aparência" do ecossistema.
- Permite alterar a tela (Watchface) do próprio M3, mas também emite comandos JSON para alterar remotamente a tela de descanso do M1, M2 e M4.
- Usa o teclado para definir parâmetros globais (ex: Tempo de tela ligada antes do idle).

## Integração
Atua como emissor de comandos. Depende da `scTransceptorESPNow` e `scMQTTLib` para atirar os JSONs contratuais para a rede, e da `scArmazenamentoLocal` para persistir os "Apelidos" e suas próprias configurações de Wi-Fi pós-validação.
