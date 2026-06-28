# scConfigOTA

## Responsabilidade no Ecossistema
A biblioteca `scConfigOTA` é o middleware responsável por gerenciar credenciais de rede, configurações base e as atualizações Over-The-Air (OTA). Ela garante que um módulo possa ser atualizado remotamente sem necessidade de acesso físico e mantém as credenciais salvas em memória não-volátil (LittleFS/NVS).

## Papel no Projeto
- Integra a biblioteca nativa `ArduinoOTA` para desenvolvimento local (recebe atualizações direto da Arduino IDE via Wi-Fi).
- **OTA em Produção (HTTPUpdate):** Possui rotinas para baixar o firmware automaticamente de um servidor via HTTP (ex: `serversilas.ddns.net/firmware/modulo2/m2.bin`).
- Verifica por novas atualizações no boot (ao ligar).
- Permite a atualização dinâmica da URL de download remoto por comando MQTT (comandado pelo Backend), salvando o novo link de forma persistente através da `scArmazenamentoLocal`.
- Foca estritamente na atualização do código, não se envolvendo com a conexão Wi-Fi em si (que é responsabilidade da `scGestorRede`).
