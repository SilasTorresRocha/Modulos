#include "scGestorDisplay.h"

scGestorDisplay::scGestorDisplay() {
    _display = nullptr;
    _paginaAtual = PAGINA_WATCHFACE;
    _ultimoFrameTempo = 0;
    
    _wifiConectado = false;
    _wifiRssi = -100;
    _sistemaMutado = false;
    _dadoValor = "--";
    _dadoUnidade = "";
    _mensagem = "Booting scOS...";
    _infoMac = "00:00:00:00:00:00";
    _infoIp = "0.0.0.0";
    _infoNomeModulo = "Modulo";
    
    _menuTitulo = "MENU";
    _menuItens = nullptr;
    _menuQtdItens = 0;
    _menuSelecionado = 0;
}

void scGestorDisplay::inicializar(U8G2* displayFisico) {
    _display = displayFisico;
    
    if (_display != nullptr) {
        _display->begin();
        _display->clearBuffer();
        _display->setFont(u8g2_font_ncenB08_tr); 
        _display->drawStr(10, 30, "System Start");
        _display->sendBuffer();
    }
}

// ----------------------------------------------------
// NAVEGAÇÃO
// ----------------------------------------------------
void scGestorDisplay::setPaginaAtual(PaginaDisplay pagina) {
    _paginaAtual = pagina;
}

void scGestorDisplay::alternarPagina() {
    if (_paginaAtual == PAGINA_WATCHFACE) _paginaAtual = PAGINA_INFO;
    else _paginaAtual = PAGINA_WATCHFACE;
}

// ----------------------------------------------------
// INJETORES DE DADOS (SETTERS)
// ----------------------------------------------------
void scGestorDisplay::setStatusRede(bool conectado, int rssi) { _wifiConectado = conectado; _wifiRssi = rssi; }
void scGestorDisplay::setDadoPrimario(String valor, String unidade) { _dadoValor = valor; _dadoUnidade = unidade; }
void scGestorDisplay::setMensagemRodape(String msg) { _mensagem = msg; }

void scGestorDisplay::setDadosMenu(const char* titulo, const char** itens, uint8_t qtdItens, uint8_t linhaSelecionada) {
    _menuTitulo = titulo;
    _menuItens = itens;
    _menuQtdItens = qtdItens;
    _menuSelecionado = linhaSelecionada;
}

void scGestorDisplay::setDadosInfo(String mac, String ip, String nomeModulo) {
    _infoMac = mac;
    _infoIp = ip;
    _infoNomeModulo = nomeModulo;
}

void scGestorDisplay::setMudo(bool mutado) {
    _sistemaMutado = mutado;
}

// ----------------------------------------------------
// MOTOR GRÁFICO
// ----------------------------------------------------
void scGestorDisplay::atualizar() {
    if (_display == nullptr) return;
    
    // FPS CAPPING: Limita a tela a desenhar no maximo a cada 100ms (10 FPS).
    uint32_t tempoAtual = millis();
    if ((tempoAtual - _ultimoFrameTempo) < 100) {
        return; 
    }
    _ultimoFrameTempo = tempoAtual;
    
    _display->clearBuffer();
    
    switch (_paginaAtual) {
        case PAGINA_WATCHFACE:
            desenharWatchFace();
            break;
        case PAGINA_INFO:
            desenharInfoCard();
            break;
        case PAGINA_MENU:
            desenharMenu();
            break;
    }
    
    _display->sendBuffer();
}

// ----------------------------------------------------
// RENDERIZADORES DE PAGINAS
// ----------------------------------------------------
void scGestorDisplay::desenharWatchFace() {
    // 1. Barra de Status (Topo) - Icone de Sinal Wi-Fi
    int startX = 2;
    int startY = 8; 
    
    if (!_wifiConectado) {
        // Sem conexao: Desenha um "X"
        _display->drawLine(startX, startY - 6, startX + 6, startY);
        _display->drawLine(startX, startY, startX + 6, startY - 6);
        _display->setFont(u8g2_font_5x8_tf);
        _display->drawStr(startX + 12, 8, "OFFLINE");
    } else {
        // Conectado: Desenha as barras baseadas no RSSI
        int barras = 0;
        if (_wifiRssi >= -60) barras = 4;      // Sinal Excelente
        else if (_wifiRssi >= -70) barras = 3; // Sinal Bom
        else if (_wifiRssi >= -80) barras = 2; // Sinal Fraco
        else if (_wifiRssi >= -90) barras = 1; // Quase caindo
        
        for (int i = 0; i < 4; i++) {
            int bx = startX + (i * 4); // Espacamento entre barras
            int bh = 2 + (i * 2);      // Altura progressiva: 2, 4, 6, 8
            int by = startY - bh + 1;
            
            if (i < barras) {
                _display->drawBox(bx, by, 3, bh);   // Barra preenchida (Tem sinal)
            } else {
                _display->drawFrame(bx, by, 3, bh); // Moldura oca (Perdeu essa barrinha)
            }
        }
    }
    
    // Icone de Mute
    if (_sistemaMutado) {
        _display->setFont(u8g2_font_5x8_tf);
        _display->drawStr(105, 8, "MUTE"); 
    }
    
    _display->drawLine(0, 10, 128, 10);
    
    _display->setFont(u8g2_font_logisoso24_tr); 
    int xDado = 10;
    _display->drawStr(xDado, 42, _dadoValor.c_str());
    
    int wValor = _display->getStrWidth(_dadoValor.c_str());
    _display->setFont(u8g2_font_6x12_tf);
    _display->drawStr(xDado + wValor + 5, 42, _dadoUnidade.c_str());
    
    _display->drawLine(0, 52, 128, 52);
    _display->drawStr(0, 62, _mensagem.c_str());
}

void scGestorDisplay::desenharInfoCard() {
    _display->setFont(u8g2_font_6x12_tf);
    _display->drawStr(0, 10, "--- INFO SISTEMA ---");
    _display->drawStr(0, 25, ("Modulo: " + _infoNomeModulo).c_str());
    _display->drawStr(0, 40, ("IP: " + _infoIp).c_str());
    _display->drawStr(0, 55, ("MAC: " + _infoMac).c_str());
}

void scGestorDisplay::desenharMenu() {
    _display->setFont(u8g2_font_6x12_tf);
    
    // Desenha o Título Dinamico (Ex: "CONF RELE", "SISTEMA")
    _display->drawStr(0, 10, _menuTitulo);
    _display->drawLine(0, 13, 128, 13);
    
    if (_menuItens == nullptr || _menuQtdItens == 0) return;
    
    // Renderiza dinamicamente apenas as opcoes recebidas do Modulo 2
    int yBase = 26;
    for (int i = 0; i < _menuQtdItens; i++) {
        // Logica de janela (Scroll). O ideal e no maximo 3 itens na tela (yBase + i*12 <= 64)
        // Para simplificar, assumimos que o Módulo 2 envia blocos de 3 itens de cada vez.
        if (i == _menuSelecionado) {
            _display->drawStr(0, yBase + (i * 14), ">"); // Cursor
        }
        _display->drawStr(10, yBase + (i * 14), _menuItens[i]);
    }
}
