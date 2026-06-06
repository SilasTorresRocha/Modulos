# scRelogioSincronizado

## Responsabilidade no Ecossistema
Esta biblioteca cuida do motor de tempo não-bloqueante do dispositivo, garantindo que o tempo real (UNIX time) esteja correto e contínuo, seja por sincronização online (NTP) ou offline (ESP-NOW).

## Papel no Projeto
- Acorda a placa no ano de 1970 e busca a hora real via NTP de forma assíncrona assim que a internet conecta.
- Aceita injeção direta de tempo caso o Hub (Módulo 3, que possui um RTC DS3231 físico) envie a hora correta via ESP-NOW durante uma queda de internet.
- Fornece o tempo contínuo para agendamentos de tarefas e envio de Telemetria de Sensores sem bloquear a execução da placa.

---

## Arquitetura e Decisões de Design (Mecanismos Avançados)

### 1. O SNTP Nativo em Background
Para evitar o uso de bibliotecas pesadas e bloqueantes como `NTPClient` (que exigem lidar com pacotes UDP manualmente na mesma Thread), a biblioteca utiliza o método `configTime()` nativo do Core do ESP. Isso delega a tarefa de "pescar" a hora nos servidores NTP diretamente para o Sistema Operacional da placa, que o faz em background sem sacrificar um único microssegundo do nosso *Super-Loop*.

### 2. Motor de Extrapolação Temporal (Software RTC)
Se o roteador queimar após o boot da placa, o SNTP parará de atualizar. Para garantir que a placa não pare no tempo, o método `obterHoraUnix()` não depende cegamente da internet. Ele atua como um **Software RTC**. 
Quando a hora é recebida pela primeira vez (NTP ou ESP-NOW), a biblioteca grava o `_timestampBase` e também a âncora `_millisSincronizacao`. Para fornecer a hora exata dali em diante, ela simplesmente calcula `millisAtual - _millisSincronizacao`, somando os segundos decorridos à base. O tempo continuará correndo com perfeição milimétrica baseado apenas no cristal oscilador de quartzo do ESP!

### 3. Injeção de "Single Source of Truth" (SSOT)
O Módulo 3 possui um RTC físico DS3231. Se o Módulo 1 (Sem internet) acordar cego, o Módulo 3 pode transmitir um pacote ESP-NOW injetando a hora. Ao chamar `definirHoraManualmente()`, a biblioteca recria a âncora temporal, priorizando a informação do ecossistema e voltando a funcionar perfeitamente sem nunca ter tocado na internet.

### 4. Blindagem Anti-Overflow de 49 Dias
A mágica de usar `uint32_t deltaMillis = (uint32_t)(millis() - _millisSincronizacao);` herda exatamente a mesma proteção matemática da biblioteca `scAgendadorTarefas`. A subtração binária garante que o relógio não retrocederá no tempo nem pulará dias caso o sistema não passe pelo Reboot Preventivo Diário e acerte a parede dos 49 dias do Arduino.
