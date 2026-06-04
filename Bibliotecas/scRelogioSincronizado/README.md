# scRelogioSincronizado

## Responsabilidade no Ecossistema
Esta biblioteca cuida do motor de tempo não-bloqueante do dispositivo, garantindo que o tempo real (UNIX time) esteja correto, seja por sincronização online ou offline.

## Papel no Projeto
- Busca a hora via NTP quando a internet está disponível.
- Busca a hora via ESP-NOW conversando com o RTC de outro módulo (modulo-03) caso a internet caia.
- Fornece métodos fáceis para a obtenção do tempo atual para uso em agendamentos, sem que o módulo se preocupe de onde o tempo está vindo, mantendo a precisão através de `millis()` internamente.
