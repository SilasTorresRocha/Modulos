# Backend (Servidor Web Docker)

**Ambiente:** Nuvem/Servidor Local em Docker

> **Nota de Dependência:** Este módulo faz parte do Ecossistema IoT. Leia o documento `Ecossistema/README.md` localizado na raiz do projeto para entender a arquitetura base.

> **O Túnel de Comunicação (scMQTTLib):** É fundamental destacar que a infraestrutura de rede MQTT (`scMQTTLib`) já é um serviço externo, independente e funcional que está rodando em produção. O Backend a ser construído aqui **não implementará pontes MQTT do zero**; sua arquitetura se baseia em consumir e utilizar a API da `scMQTTLib` como seu canal de transmissão de pacotes e recepção de telemetria, mantendo as lógicas de negócio separadas do duto de transporte.

## Papel no Ecossistema
O Backend atua como a **Interface Principal e Remota** de todo o ecossistema. Suas responsabilidades (Front-end e API) espelham as funções do Módulo 3 (Central), porém voltadas para o acesso seguro via internet e armazenamento de longo prazo (Banco de Dados).

## Recursos Principais
*   **Gestão via Web:** Site acessado via senha com Dashboard completo de toda a casa.
*   **Gerenciador de Mqtt:** Recebe a telemetria, salva os status (Online, Offline, Inativo) e repassa os comandos de ações reativas.
*   **Controlador Master (Acesso Remoto Completo):** Toda configuração possível do sistema pode ser feita aqui. As calibrações de sensores (ex: MQ-2), os agendadores e estados de retorno pós-queda de energia (Relés Módulo 2), cadastro de novos SSIDs de Wi-Fi e a gestão de banco de usuários do Cofre (Módulo 4) fluem a partir daqui.
*   **Provisionamento de Chave Simétrica (A Senha da Casa):** O Backend deve permitir ao usuário criar uma senha mestre forte. O Backend converterá isso matematicamente (hash/padding) em uma chave de *exatos 16 bytes* (para PMK/LMK do ESP-NOW) e a enviará (via comando MQTT `update_espnow_key`) para que os módulos salvem na memória física, garantindo a criptografia no modo offline sem internet.
*   **Servidor de OTA (Over-The-Air):** Deve fornecer e hospedar as rotas HTTP estáticas para o download dos binários de atualização dos módulos (ex: `GET /firmware/modulo2/m2.bin`), suportando a rotina de atualização automática a cada 24 horas feita pelos módulos.

## Snippets de Teste (Repasse de Telemetria via MQTT)

Quando o Módulo 1 (Forno) e o Módulo 2 (Relés) estão fora de alcance do rádio ESP-NOW local, o Backend deve repassar a telemetria do M1 (como alertas de Gás ou Temperatura) para o M2 através do tópico de comandos do usuário (ex: `comandos/silastorres`).

**Limpar Alarme de Gás:**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "repassar_telemetria",
  "args": {
    "tipo": "M1",
    "gas": 200
  }
}
```

**Disparar Alarme de Gás:**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "repassar_telemetria",
  "args": {
    "tipo": "M1",
    "gas": 1500
  }
}
```

**Repassar Temperatura do M1 (Para exibir na UI do M2):**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "repassar_telemetria",
  "args": {
    "tipo": "M1",
    "temp": 29.5
  }
}
```

## Snippets de Configuração (Módulo 1)

O Módulo 1 (MAC: `E8:9F:6D:93:3F:34`) possui comandos de configuração de parâmetros internos que devem ser implementados no Dashboard do Backend. Todos são persistentes (salvos na Flash da placa).

**Configurar Limite (Sensibilidade) do Gás MQ-2:**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "E8:9F:6D:93:3F:34",
  "cmd": "configurar_limiar_gas",
  "args": {
    "limiar": 800
  }
}
```

**Configurar Operação do Forno / Interface / Alarmes:**
> O `alm_prep` é um temporizador de cozinha independente (desliga com `0`). O `alm_crit` dispara a sirene se o forno ficar ligado por X segundos (limite de segurança contra esquecimento).
```json
{
  "mac_origem": "HUB",
  "mac_destino": "E8:9F:6D:93:3F:34",
  "cmd": "configurar_operacao",
  "args": {
    "tela_idle": 1,
    "alm_prep": 600,
    "alm_crit": 1800,
    "temp_abs": 65.0,
    "temp_der": 1.5
  }
}
```

**Forçar Envio de Telemetria (Útil para painel em Real-Time):**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "E8:9F:6D:93:3F:34",
  "cmd": "solicitar_status",
  "args": {}
}
```

## Snippets de Configuração (Módulo 2)

O Módulo 2 (Relés Inteligentes e Interface) também aceita comandos do Backend. MAC base de exemplo: `AA:BB:CC:DD:EE:22`.

**Forçar Liga/Desliga de um Relé remotamente:**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "set_rele",
  "args": {
    "id": 1,
    "estado": true
  }
}
```

**Configurar Comportamento Físico de um Relé (Persistente):**
- `pot_w`: Potência estimada (Ex: 1500W) ligada a este relé (para cálculo de KWh).
- `fb_gas`: Fallback de Gás (1 = Desliga em emergência de gás, 0 = Ignora gás, 2 = Força LIGAR em emergência de gás - Ex: Exaustor).
- `ret_pwr`: Return of Power State (Ação após queda de luz): 1 = Volta Ligado, 2 = Volta Desligado, 3 = Lembra o último estado.
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "configurar_rele",
  "args": {
    "id": 1,
    "pot_w": 1500.0,
    "fb_gas": 1,
    "ret_pwr": 3
  }
}
```

> **Dica Final de Rede:** O comando `configurar_limiar_gas` ensinado no snippet do M1 também é escutado pelo M2. Recomenda-se enviá-lo com `"mac_destino": "ALL"`, assim os dois módulos sincronizam o que eles consideram ser o ponto crítico de acionamento do alarme geral e bloqueio de relés.
