# scGestorDispositivosM3 (O Tradutor de Nomes)

## Responsabilidade
Manter a experiência do usuário (UX) intuitiva através do mapeamento semântico entre endereços MAC complexos e nomes humanizados (Apelidos).

## Princípio (Camada de Identidade Visual)
O ecossistema em seu baixo nível opera unicamente roteando tráfego via endereços MAC (ex: `11:22:33:AA:BB:CC`). Entretanto, desenhar na tela "Falha no 11:22:33" destrói a usabilidade da casa inteligente. Esta biblioteca atua como uma tabela DNS local.

## Contratos
- Lê um arquivo de configuração gravado na Flash ou SD Card contendo um dicionário de equivalência.
- Aloca esse dicionário leve em memória RAM.
- **Função Principal:** Fornece o método estrito que recebe uma String/char array de um MAC e devolve o Nome Amigável configurado pelo usuário (ex: `"Luzes do Jardim"`).
- Se não houver nome cadastrado para aquele MAC, retorna o próprio MAC ou o nome da Classe Genérica (ex: `"Módulo 2 (AA:BB)"`).
- É altamente consumida pela `scGestorTelasM3` antes de invocar a renderização gráfica de qualquer popup de falha ou menus de configuração.
