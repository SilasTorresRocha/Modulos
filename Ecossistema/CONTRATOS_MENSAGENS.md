# Contratos de Mensageria (A Bíblia de Comunicação)

Este documento dita a estrutura de dados que trafega entre os Módulos, o Módulo Central (Hub) e o Backend Web.
Como a biblioteca `scMQTTLib` funciona como um canal estrito de TX/RX, o roteamento de origem e destino é feito **dentro do JSON**.

## 1. Camada de Aplicação vs. Transporte

Este documento dita a **Camada de Aplicação** (o formato do texto/JSON). A forma como essa mensagem viaja (via MQTT ou via rádio ESP-NOW) é a Camada de Transporte.
*A regra de ouro:* As duas rotas de recepção (o callback do MQTT e o callback do ESP-NOW) devem desembocar na **mesma função de processamento JSON** em C++. O microcontrolador lê o JSON de entrada e reage, sem se importar por qual "antena" a mensagem chegou.

## 2. Limite de Bytes do ESP-NOW (Minificação de Chaves)

O payload máximo de um pacote ESP-NOW é de **250 bytes**. Como fragmentação (dividir pacotes) gera um overhead terrível em microcontroladores e atrasa o loop, a solução é **Minificar as Chaves do JSON** e usar a função `serializeJson()` da ArduinoJson (que envia tudo numa linha sem espaços).

> **Atenção:** Em vez de enviar `"alarme_critico_esquecimento_restante_segundos": 1800`, enviaremos `"alm_crit": 1800`.

---

## 3. Telemetria e Heartbeat (Saída dos Módulos para o Server)

Sempre que um módulo publica, o `mac_destino` é implicitamente o "HUB/SERVER".

### 3.1 Telemetria Constante (Heartbeat)

#### Módulo 1 (Forno e Gás)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:11",
  "tipo": "M1",
  "dados": {
    "temp": 28.5,            // Temperatura atual
    "gas": 300,              // Nível de gás
    "tx_temp": 0.5,          // Taxa de variação de temperatura (°C/s)
    "t_forno": 1200,         // Tempo de forno ligado (segundos)
    "alm_prep": 600,         // Tempo restante alarme de preparo (segundos)
    "alm_crit": 1800,        // Tempo restante alarme crítico (segundos)
    "status": "online",
    "rssi": -65,             // Qualidade do sinal de rede atual
    "err": []                // Array de erros críticos (ex: ["falha_mq2"])
  }
}
```

#### Módulo 2 (Relés e Encoder)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:22",
  "tipo": "M2",
  "dados": {
    "r1_st": true,           // Estado do Relé 1
    "r1_t": 3600,            // Tempo ligado na sessão (segundos)
    "r1_kw": 1.5,            // Consumo acumulado KWh
    "r2_st": false,          // Estado do Relé 2
    "r2_t": 0,
    "r2_kw": 0.0,
    "tot_kw": 1.5,           // Consumo Total acumulado
    "status": "online",
    "rssi": -70,
    "err": []
  }
}
```

#### Módulo 3 (Central / Hub)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:33",
  "tipo": "M3",
  "dados": {
    "temp_rtc": 24.5,        // Temperatura lida no sensor físico DS3231
    "status": "online",
    "rssi": -50,
    "err": []
  }
}
```

#### Módulo 4 (Biometria)
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:44",
  "tipo": "M4",
  "dados": {
    "t_fechada": true,       // Status do sensor da trava solenoide
    "ult_id": 4,             // Último ID que acessou
    "status": "online",
    "rssi": -60,
    "err": ["falha_sensor_solenoide"] // Exemplo de falha de hardware
  }
}
```

### 3.2 Eventos Pontuais (Callbacks Assíncronos)
Usado quando o módulo precisa confirmar uma ação isolada (diferente da telemetria que fica rodando no loop). Isso é útil para o Backend exibir pop-ups para o usuário.

**Exemplo: Módulo 4 avisando o resultado do cadastro de um dedo:**
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:44",
  "tipo": "M4",
  "evento": "resultado_cadastro",
  "dados": {
    "sucesso": true,
    "id_cadastrado": 5,
    "msg": "Digital gravada com sucesso"
  }
}
```

**Exemplo: Módulo 2 confirmando (ACK) a alteração do Relé:**
```json
{
  "mac_origem": "AA:BB:CC:DD:EE:22",
  "tipo": "M2",
  "evento": "rele_alterado",
  "dados": {
    "id": 1,
    "estado": true
  }
}
```

---

## 4. Comandos e Configurações (Entrada nos Módulos)

Todo módulo ao ler `mqtt_sc.obterComando()` deve desserializar o JSON e verificar se `mac_destino` bate com seu MAC ou com `"ALL"`. Se não bater, descarta o pacote.

*Nota de Implementação (C++):* O comando "configurar_operacao" pode enviar apenas as chaves que foram alteradas pelo usuário. Use a função `containsKey()` da biblioteca ArduinoJson para testar o **Objeto Parcial**. Exemplo:
```cpp
if (doc["args"].containsKey("alm_crit")) {
    tempo_alarme_esquecimento = doc["args"]["alm_crit"];
}
```

### 4.1 Comandos Universais (Obrigatórios em todos os módulos)

**Atualizar Wi-Fi (Broadcast):**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "update_wifi",
  "args": {
    "ssid": "RedeCasa",
    "pass": "12345"
  }
}
```

**Sincronizar Relógio (Offline NTP via ESP-NOW):**
Emitido pelo Módulo 3 (que possui RTC DS3231) para garantir agendamentos sem internet.
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "sincronizar_relogio",
  "args": {
    "timestamp_unix": 1781258400,
    "offset_h": -3.0         // Fuso horário em horas (suporta float, ex: +5.5 para Índia)
  }
}
```

**Provisionamento de Endereços (Discovery P2P):**
Informa a um módulo qual é o MAC Address do seu parceiro direto (ex: Avisa o Módulo 1 quem é o Módulo 2 para o caso de vazamento de gás). Também é usado para avisar aos módulos periféricos qual é o MAC Address do próprio Hub.
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:11",
  "cmd": "set_peer_mac",
  "args": {
    "tipo_alvo": "M2",
    "mac_alvo": "AA:BB:CC:DD:EE:22"
  }
}
```

**Atualizar Credenciais da Lib MQTT (NVS + Restart):**
Ao receber isso, o ESP salva em memória não-volátil ou algo com LittleFS e aplica `ESP.restart()`.
```json
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "update_libmqtt",
  "args": {
    "usuario": "NovoUsuario",
    "senha": "NovaSenha"
  }
}
```

**Manutenção (Ping e Reboot):**
```json
// Comando para forçar um módulo a reiniciar fisicamente (Soft Reset)
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:11",
  "cmd": "reiniciar_dispositivo",
  "args": {}
}
```

```json
// Comando para forçar módulos a enviarem telemetria instantânea (Ignora intervalo de envio)
{
  "mac_origem": "HUB",
  "mac_destino": "ALL",
  "cmd": "solicitar_status",
  "args": {}
}
```

**OTA (Over-The-Air Update):**
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:11",
  "cmd": "atualizar_firmware",
  "args": {
    "url_binario": "http://servidor/firmwares/modulo1_v2.bin"
  }
}
```

### 4.2 Comandos Específicos: Módulo 1 (Forno/Gás)
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:11",
  "cmd": "configurar_operacao",
  "args": {
    "tela_idle": 2, 
    "sens_gas": 500,
    "alm_prep": 1800,  //tempo_alarme_preparo_segundos
    "alm_crit": 3600   //tempo_alarme_esquecimento_segundos
  }
}
```

### 4.3 Comandos Específicos: Módulo 2 (Relés)
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "set_rele",
  "args": {"id": 1, "estado": true}
}
```
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "configurar_rele",
  "args": {
    "id": 1,
    "pot_w": 1500,           // Potência da carga conectada (Watts) para cálculo de consumo
    "fb_gas": 2              // Regra de Gás: 1 (Ignorar), 2 (Bloquear/Desligar), 3 (Forçar Exaustor)
  }
}
```
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:22",
  "cmd": "reset_consumo",
  "args": {"id_rele": 1}     // 1, 2 ou "ALL"
}
```

### 4.4 Comandos Específicos: Módulo 4 (Biometria)
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:44",
  "cmd": "cadastrar_digital",
  "args": {"novo_id": 5}
}
```
```json
{
  "mac_origem": "HUB",
  "mac_destino": "AA:BB:CC:DD:EE:44",
  "cmd": "apagar_digital",
  "args": {"id_alvo": 5} // Enviar 0 ou "ALL" para apagar o banco inteiro
}
```
