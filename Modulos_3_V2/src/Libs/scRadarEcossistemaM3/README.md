# scRadarEcossistemaM3 (Controle da Malha)

## Responsabilidade
Monitorar de forma passiva, constante e autônoma a "vida e a morte" de todos os submódulos (M1, M2, M4) do ecossistema doméstico.

## Princípio (Dead Man's Switch / Interruptor de Homem Morto)
Em um sistema crítico de detecção (como vazamento de gás do Módulo 1), não receber mensagens é tão grave quanto receber uma mensagem de alarme. O silêncio implica falha. Esta biblioteca existe para agir sobre esse silêncio.

## Contratos e Regras
- O Radar não atira pacotes perguntando ativamente "você está aí?" para economizar energia da malha. Ele age de forma passiva.
- Ele possui uma tabela Hash / Array de Structs que cruza os MAC Addresses conhecidos com os *Timestamps* (hora exata do último sinal de telemetria recebido pelo Roteador) e a *Origem* do sinal (Wi-Fi ou ESP-NOW).
- A cada ciclo de Loop (controlado pelo agendador), ele processa a lógica trifásica de cores:
  - 🟢 **Verde (Online):** `millis() - ultimo_ping < 60s`. Módulo perfeitamente saudável no Wi-Fi.
  - 🟡 **Amarelo (Fallback):** `millis() - ultimo_ping > 60s` **E** o último ping recebido foi categorizado como ESP-NOW pelo Roteador. Indica que o nó perdeu a Nuvem, mas a malha local continua ouvindo.
  - 🔴 **Vermelho (Morto/Inativo):** `millis() - ultimo_ping > 300s`. O Radar conclui que o Módulo não tem energia ou está fisicamente quebrado.
- **Ação em Falha:** Imediatamente decreta "Emergência de Ponto Cego" na casa. A biblioteca levanta flags, notifica a Nuvem e muda a UI via `scGestorTelasM3` para o modo de alerta e bloqueio preventivo.
