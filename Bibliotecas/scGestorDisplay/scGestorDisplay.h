#ifndef SC_GESTOR_DISPLAY_H
#define SC_GESTOR_DISPLAY_H

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

enum PaginaDisplay {
    PAGINA_WATCHFACE, // Tela Principal (ex: Valores Grandes)
    PAGINA_INFO,      // Tela Secundaria (ex: Status de IP, MAC, Erros)
    PAGINA_MENU       // Tela de Menu Dinâmico
};

class scGestorDisplay {
public:
    scGestorDisplay();
    
    // Injeta a tela fisica ja construida no .ino.
    void inicializar(U8G2* displayFisico);
    
    // O motor grafico de frames. Deve rodar no loop.
    void atualizar();
    
    // ======================================
    // API de Navegação
    // ======================================
    void setPaginaAtual(PaginaDisplay pagina);
    void alternarPagina(); 
    
    // ======================================
    // API de Injeção de Dados (Setters - View)
    // ======================================
    void setStatusRede(bool conectado, int rssi = -100);
    void setDadoPrimario(String valor, String unidade);
    void setMensagemRodape(String msg);
    
    // O GestorDisplay atua apenas como "View" (Impressora visual).
    // A logica pesada de submenus (Model/Controller) fica no Módulo 2, 
    // que injeta apenas os textos a serem desenhados na tela neste exato momento.
    void setDadosMenu(const char* titulo, const char** itens, uint8_t qtdItens, uint8_t linhaSelecionada);
    
    // Injeta os dados da aba de diagnóstico
    void setDadosInfo(String mac, String ip, String nomeModulo);
    
    // Altera a visibilidade do icone de Mute
    void setMudo(bool mutado);

private:
    U8G2* _display;
    PaginaDisplay _paginaAtual;
    
    // Controle de FPS para não saturar o barramento I2C
    uint32_t _ultimoFrameTempo;
    
    // Armazenamento de Estado (Cache visual)
    bool _wifiConectado;
    int _wifiRssi;
    bool _sistemaMutado;
    String _dadoValor;
    String _dadoUnidade;
    String _mensagem;
    String _infoMac;
    String _infoIp;
    String _infoNomeModulo;
    
    // Cache visual do Menu Dinamico
    const char* _menuTitulo;
    const char** _menuItens;
    uint8_t _menuQtdItens;
    uint8_t _menuSelecionado;
    
    // Rotinas internas de desenho de pixels
    void desenharWatchFace();
    void desenharInfoCard();
    void desenharMenu();
};

#endif // SC_GESTOR_DISPLAY_H
