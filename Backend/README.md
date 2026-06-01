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
