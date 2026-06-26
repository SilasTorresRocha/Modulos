# scMonitorSaudeM1 (Módulo 1)

## Responsabilidade Central
Atuando como o "médico residente" da placa Módulo 1 (focada em segurança primária e saúde de detecção de fogo e gás), esta biblioteca realiza o monitoramento de hardware físico do sistema de sensores. Herda de `scSaudeHardware`.

## Princípios Adotados

### 1. Ping I2C (0x3C)
Verifica se o display OLED continua conectado ao barramento. Assim como no M2, se a conexão for perdida, as lógicas de renderização de tela são avisadas para não travarem o loop com timeouts e evitar congelamentos.

### 2. Ping 1-Wire (DS18B20)
O sensor de temperatura DS18B20 pode ser exposto a temperaturas extremas ou ter seu cabo derretido/desconectado. Esta classe consulta ciclicamente se o endereço físico do sensor ainda está ativo no barramento 1-Wire.

### 3. Sanidade Analógica do MQ-2 (Pino A0)
O sensor de gás gera algum "ruído" base em ar limpo (geralmente uma leitura entre 30 e 50 no ADC). Se a leitura no pino A0 cair cravada em 0 absoluto por repetidos ciclos, isso indica cabo rompido ou falha grave no sensor analógico.

### 4. Telemetria
Alimenta o vetor de erros globais com strings contratuais como `["i2c_morto", "ds18b20_desconectado", "mq2_falha_analogica"]` para que a `scTelemetriaM1` despache ao Backend/Hub.

## Dependências
- `scSaudeHardware` (Base)
- `Wire` (I2C)
- `OneWire` e `DallasTemperature` (para health check 1-Wire)
