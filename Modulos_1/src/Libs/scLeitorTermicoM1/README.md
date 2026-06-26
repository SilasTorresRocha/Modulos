# scLeitorTermicoM1 (Módulo 1)

## Responsabilidade Central
Atua como o motor de processamento matemático para a análise térmica do forno, monitorando a evolução de temperatura.

## Princípios Adotados

### 1. Leitura Assíncrona e Não Bloqueante
A leitura de sensores no barramento 1-Wire via DS18B20 tipicamente trava o microcontrolador por até 750ms esperando conversão (se usada de forma ingênua). A biblioteca usa  `setWaitForConversion(false)`.
Ela solicita a temperatura e volta no próximo loop para colher os dados, não congelando as funções críticas, como leitura de vazamentos de gás.

### 2. Motor de Derivada Térmica (°C/s)
Compara a temperatura atual com a última lida dividida pela diferença temporal em segundos (`millis()`). Esta taxa de aquecimento permite inferir que o "forno foi ligado", mesmo se ainda estiver morno, identificando picos de variação fora do padrão climático normal.

### 3. Ponto de Ruptura
Aciona a flag interna `_fornoLigado = true` ao detectar uma taxa de aquecimento rápida, OU caso ultrapasse a temperatura limite de segurança estipulada em tempo de configuração (55°C).

## Dependências
- `OneWire` e `DallasTemperature`
