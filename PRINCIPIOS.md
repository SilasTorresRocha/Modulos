# Princípios de Desenvolvimento do Ecossistema

Este documento serve como o norte (guia) para a arquitetura, desenvolvimento e evolução de todo o projeto. Qualquer novo módulo, funcionalidade ou backend adicionado deve obedecer a estes princípios fundamentais.

## 1. Redundância e Resiliência (Tolerância a Falhas)
O sistema foi concebido para ser o mais resistente possível. Se o Hub (Módulo Central) ou o Servidor Web (Backend) caírem, a casa não pode parar de funcionar. 
*   **Descentralização de Tarefas Críticas:** Funções vitais devem ter fallback local. Por exemplo, os agendamentos semanais do Módulo 2 são **salvos fisicamente na placa (NVS/LittleFS)**, permitindo que os relés operem mesmo sem comunicação externa.
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
*   **MAC Address como Chave Primária:** Não há o conceito de ID dinâmico dependente de um servidor DHCP interno. A identidade de uma placa é sempre o seu Endereço MAC físico.
*   O Hub e o Backend gerenciam a tradução amigável desses MACs para os usuários (através de "Apelidos" configuráveis), mantendo o roteamento por trás dos panos sólido e imutável.

## 5. Abstração de Interface Local (Watch Faces e Info)
Telas e displays locais (como OLEDs e TFTs) devem priorizar o design limpo (*Watch faces*). Informações técnicas e de diagnóstico (como IP, status detalhado de conexão e Endereço MAC para identificação no Hub) devem ser segregadas em abas específicas de "Informações do Sistema" (INF). É permitido ter múltiplas opções de watch faces para o usuário.

## 6. Padronização Global
Qualquer novo módulo inserido na rede deve seguir estritamente os padrões de alertas sonoros (Buzzer), de display (`scGestorDisplay`) e as máquinas de estado, não agindo como um "corpo estranho" para o usuário. A experiência da casa é unificada.

## 7. Segurança Descentralizada e Criptografia
A arquitetura presume que as redes locais não são seguras por natureza.
*   **A "Senha da Casa" (Chave Simétrica):** O Backend e o Hub injetam fisicamente nos módulos uma chave AES de 16 bytes para criptografia LMK/PMK.
*   **Comunicação Indestrutível:** No modo de Fallback (sem roteador/offline), as antenas sobem com criptografia de hardware nativa. Isso garante que invasores capturando pacotes no ar ou injetando MACs clonados não consigam ler o tráfego.
*   **Anti-Replay Attack (Timestamp):** Para dispositivos críticos (Cofre/Biometria), a criptografia não basta. O Backend/Hub deve enviar a chave `"ts"` (Timestamp) no JSON. O módulo receptor cruza a data do pacote com o seu próprio `scRelogioSincronizado`. Se a diferença de tempo for maior que uma janela de aceitação (ex: 5s), o comando é descartado, invalidando clones de pacotes gravados no ar.
*   **Revogação Física:** O backend possui o poder de desativar módulos que foram furtados (revogando seu MAC), provendo controle central sobre ativos perdidos.

## 8. Qualidade Absoluta e Nenhuma Retrocompatibilidade (Fase de Desenvolvimento)
Durante a fase de desenvolvimento atual, **não há compromisso com retrocompatibilidade**. As bibliotecas core (`sc*`) são projetadas para alcançar o mais alto padrão de qualidade (nível industrial/aeroespacial). 
*   Se uma assinatura de método precisar ser alterada para melhorar performance (ex: uso de ponteiros constantes no lugar de cópias de String) ou para garantir segurança, ela será alterada diretamente. 
*   Não são permitidas "gambiarras" ou sobrecargas obsoletas apenas para não quebrar códigos antigos. Se uma API mudar, os módulos dependentes deverão ser reescritos para se adequarem ao novo e melhor padrão.
