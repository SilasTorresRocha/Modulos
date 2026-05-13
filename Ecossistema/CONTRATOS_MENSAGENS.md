# Contratos de Mensageria (A Bíblia de Comunicação)

Este documento dita a estrutura rígida de dados que trafega entre os Módulos, o Módulo Central (Hub) e o Backend Web.

Como a biblioteca `scMQTTLib` funciona como um canal estrito de TX/RX (Topologia de Barramento Aberto), a diferenciação de quem enviou e para quem a mensagem é destinada **não é feita pelo Tópico MQTT**, mas sim internamente no corpo do JSON.

## 1. Padrão de Roteamento Base (Routing)

Todos os módulos escutam o barramento (`comandos/seu_usuario`) e enviam dados para o barramento (`telemetria/seu_usuario`).

Para que um módulo não processe o comando de outro, todos os JSONs devem conter um cabeçalho de roteamento:
*   `mac_origem`: O endereço MAC de quem gerou o pacote.
*   `mac_destino`: O endereço MAC de quem deve ler o pacote (Use `"ALL"` para Broadcast).
*   `tipo`: O identificador do módulo ("M1", "M2", "M3", "M4" ou "HUB").

---

## 2. Telemetria e Heartbeat (Saída dos Módulos para o Server)

Quando um módulo publica, o `mac_destino` é implicitamente o "HUB/SERVER".

### Módulo 1 (Forno e Gás)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:11",
  "tipo": "M1",
  "dados": {
    "temperatura": 28.5,
    "gas": 300,
    "forno_ligado_segundos": 1200,
    "alarme_preparo_restante_segundos": 600,
    "alarme_critico_esquecimento_restante_segundos": 1800,
    "status": "online"
  }
}
```

### Módulo 2 (Relés e Encoder)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:22",
  "tipo": "M2",
  "dados": {
    "reles": {
      "r1_estado": true,
      "r1_tempo_ligado_segundos": 3600,
      "r2_estado": false,
      "r2_tempo_ligado_segundos": 0
    },
    "consumo": {
      "r1_kwh": 1.5,
      "r2_kwh": 0.0,
      "total_kwh": 1.5
    },
    "status": "online"
  }
}
```

### Módulo 4 (Biometria)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:44",
  "tipo": "M4",
  "dados": {
    "trava_fechada": true,
    "ultimo_id_acesso": 4,
    "status": "online"
  }
}
```

---

## 3. Comandos e Configurações (Entrada nos Módulos)

Todo módulo ao ler `mqtt_sc.obterComando()` deve desserializar o JSON e verificar se `mac_destino` bate com seu MAC ou com `"ALL"`. Se não bater, descarta o pacote.

### 3.1 Comandos Universais (Todos os Módulos)
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "update_wifi",
  "args": {
    "ssid": "MinhaRede Nova",
    "pass": "Senh@Forte"
  }
}
```

### 3.2 Comandos Específicos: Módulo 1
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:11",
  "cmd": "configurar_operacao",
  "args": {
    "tela_idle_modo": 2, 
    "sensibilidade_gas": 500,
    "tempo_alarme_preparo_segundos": 1800,
    "tempo_alarme_esquecimento_segundos": 3600
  }
}
```

### 3.3 Comandos Específicos: Módulo 2
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
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "reset_consumo",
  "args": {
    "id_rele": 1  // Pode ser 1, 2 ou "TODOS"
  }
}
```

### 3.4 Comandos Específicos: Módulo 4
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:44",
  "cmd": "cadastrar_digital",
  "args": {
    "novo_id": 5
  }
}
```
