# scGestorDisplay

## Responsabilidade no Ecossistema
A biblioteca `scGestorDisplay` atende ao **Princípio 5** do projeto ("Abstração de Interface Local"), controlando a atualização de telas OLEDs I2C de forma assíncrona, bela e fluida.

## Papel no Projeto
- Focada exclusivamente em displays OLED mono (como SSD1306 e SH1106).
- Gerencia o loop de desenho do display sem bloquear a execução principal da placa.
- O módulo principal apenas injeta os "Dados" (variáveis de estado) e a biblioteca cuida de convertê-los fisicamente em pixels de forma otimizada.

---

## Arquitetura e Decisões de Design

### Injeção de Dependência (U8g2)
Para resolver a incompatibilidade de hardware entre o **Módulo 1** (que usa tela 0.96" SSD1306) e o **Módulo 2** (que usa tela 1.3" SH1106), a biblioteca abraçou o poder da `U8g2`. Em vez de instanciar o hardware internamente, o método `inicializar(U8G2* displayFisico)` exige que o arquivo `.ino` passe o ponteiro da tela já configurada. Assim, a biblioteca se torna 100% agnóstica ao hardware físico e foca apenas em gerenciar a geometria da interface.

### O Padrão MVC (Model-View-Controller) nos Menus
Diferente de sistemas que engessam as opções do menu dentro da classe da tela, a `scGestorDisplay` atua puramente como **View**. Ela possui o método genérico `setDadosMenu(titulo, arrayDeItens, qtd, linhaSelecionada)`. 
A lógica complexa de submenus, estados de relé e acionamento fica no arquivo `.ino` (O **Controller**), garantindo que Módulos sem menu (M1) ou com menus gigantes (M2) usem exatamente a mesma biblioteca base.

### Capping de FPS (Otimização I2C)
Displays OLED via I2C são barramentos lentos. Se desenhar a tela em todos os ciclos do `loop()`, o chip do ESP não terá tempo para processar o Wi-Fi ou ler giros rápidos do Encoder. A função `atualizar()` trava o desenho em **10 FPS** (100ms), preservando os ciclos de CPU para o que realmente importa e mantendo a interface ainda fluida.

### Gráfico de Wi-Fi Dinâmico (Sem Bitmaps)
Para manter o aspecto refinado sem gastar bytes preciosos da memória Flash com imagens Bitmap, a barra de status traduz matematicamente a potência de sinal (`_wifiRssi`). 
O código usa formas geométricas primárias (`drawBox` para sinal ativo e `drawFrame` para as barras oca/sem sinal) criando o famigerado gráfico de escadinha com 4 níveis. Se cair, ele desenha um "X" cruzado automaticamente.

### InfoCard e UX Acústica Visível
Todos os dados de tela são dinâmicos (evitando *Hardcodes*). O método `setDadosInfo()` permite ao HUB e ao Administrador verem o IP real e o MAC da placa localmente. Além disso, a função `setMudo()` espelha a integração da placa com a `scAvisosSonoros`. Se o usuário mutar fisicamente os bipes de navegação, a tela estampará **"MUTE"** no canto superior, mantendo total transparência do estado do sistema para o usuário final.
