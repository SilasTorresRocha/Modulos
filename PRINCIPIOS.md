# Princípios de Desenvolvimento do Ecossistema

Este documento serve como o norte (guia) para a arquitetura, desenvolvimento e evolução de todo o projeto. Qualquer novo módulo, funcionalidade ou backend adicionado deve obedecer a estes princípios fundamentais.

## 1. Redundância e Resiliência (Tolerância a Falhas)
O sistema foi concebido para ser o mais resistente possível. Se o Hub (Módulo Central) ou o Servidor Web (Backend) caírem, a casa não pode parar de funcionar. 
*   **Descentralização de Tarefas Críticas:** Funções vitais devem ter fallback local. Por exemplo, os agendamentos semanais do Módulo 2 são **salvos fisicamente na placa (EEPROM/LittleFS)**, permitindo que os relés operem mesmo sem comunicação externa.
*   **Fallback de Rede Híbrida:** Quando o roteador da casa falha, os módulos assumem automaticamente uma rede secundária (ESP-NOW) para manter a comunicação interna.

## 2. Comunicação via Contratos Estritos (JSON)
Devido à complexidade do projeto e ao desenvolvimento assíncrono (diferentes módulos e tempo de desenvolvimento diferente), a comunicação nunca deve ser baseada em "achismos" de strings puras.
*   **Padronização JSON:** Toda a telemetria, configurações e comandos trafegados via MQTT ou ESP-NOW devem seguir **Contratos JSON**.
*   Isso garante que um módulo saiba exatamente qual chave buscar no pacote de outro, evitando falhas de segmentação e permitindo que componentes sejam construídos de forma independente, focando apenas no contrato de interface.

## 3. Infraestrutura Agnóstica (O Túnel Independente)
A biblioteca base de comunicação, **`scMQTTLib`**, foi desenhada com um propósito geral e independente. Ela atua estritamente como um túnel ou canal de dados entre o hardware e o broker.
*   **Separação de Regras de Negócio:** Nenhuma regra específica deste projeto de automação (ex: "verificar temperatura da cidade" ou "abrir cofre") deve ser codificada dentro da infraestrutura da `scMQTTLib`(Regra de negócio dentro da API, as regra deve ser exclusivamentes no Backend desse projeto a `scMQTTLib`(Lib + API) corresponde apenas ao canal de comunicação e envio/recebimento de mensagens). 
*   O Backend Web é o verdadeiro "cérebro" integrador. Ele usa as rotas HTTPS da API para injetar comandos que serão traduzidos para o MQTT.

## 4. Escalabilidade Horizontal e Identificação Transparente
O ecossistema é preparado para crescer de 1 para N módulos idênticos (ex: 5 Módulos 2 na mesma casa).
*   **MAC Address como Chave Primária:** Não há o conceito de ID dinâmico dependente de um servidor DHCP interno frágil. A identidade de uma placa é sempre o seu Endereço MAC físico.
*   O Hub e o Backend gerenciam a tradução amigável desses MACs para os usuários (através de "Apelidos" configuráveis), mantendo o roteamento por trás dos panos sólido e imutável.

## 5. Abstração de Interface Local (Watch Faces e Info)
Telas e displays locais (como OLEDs e TFTs) devem priorizar o design limpo (*Watch faces*). Informações técnicas e de diagnóstico (como IP, status detalhado de conexão e Endereço MAC para identificação no Hub) devem ser segregadas em abas específicas de "Informações do Sistema" (INF). É permitido ter múltiplas opções de watch faces para o usuário.
