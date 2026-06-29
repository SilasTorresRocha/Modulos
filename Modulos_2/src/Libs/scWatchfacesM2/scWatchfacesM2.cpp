#include "scWatchfacesM2.h"
#include <math.h>

scWatchfacesM2::scWatchfacesM2() {
    _logger = nullptr; _display = nullptr; _relogio = nullptr; _reles = nullptr; _telemetria = nullptr;
}

void scWatchfacesM2::inicializar(scLogger* logger, scGestorDisplay* display, scRelogioSincronizado* relogio, scGestorReles* reles, scTelemetriaM2* telemetria) {
    _logger = logger; _display = display; _relogio = relogio; _reles = reles; _telemetria = telemetria;
    
    if (_logger && (!_display || !_display->getU8G2())) {
        _logger->erro("WATCHFACES", "Falha critica: Motor U8G2 nao injetado no construtor!");
    }
}

void scWatchfacesM2::desenharWatchface(uint8_t id_watchface) {
    if (!_display || !_display->getU8G2()) {
        static bool erroLogado = false;
        if (_logger && !erroLogado) {
            _logger->erro("WATCHFACES", "Ponteiro Grafico nulo durante loop de desenho da Watchface!");
            erroLogado = true; // Impede ficar spamando no terminal
        }
        return;
    }
    
    switch (id_watchface) {
        case 0: _desenharRelogioAnalogico(); break;
        case 1: _desenharRelogioDigital(); break;
        case 2: _desenharDashboard(); break;
        case 3: _desenharSimples(); break;
        case 4: /* Tela apagada */ break;
        default: _desenharRelogioDigital(); break;
    }
}

// ====================================================================
// TELAS OCIOSAS (WATCHFACES)
// ====================================================================

void scWatchfacesM2::_desenharRelogioAnalogico() {
    U8G2* u8g2 = _display->getU8G2();
    
    int cx = 64; int cy = 32; int r = 30;
    u8g2->drawCircle(cx, cy, r);
    
    int hora = 0; int min = 0; int seg = 0; 
    if (_relogio && _relogio->estaSincronizado()) {
        uint32_t unixTime = _relogio->obterHoraUnix();
        seg = unixTime % 60;
        min = (unixTime % 3600) / 60;
        hora = (unixTime % 86400) / 3600;
    }
    
    float aSeg = (seg * 6.0) - 90.0;
    float aMin = (min * 6.0) + (seg * 0.1) - 90.0;
    float aHor = (hora * 30.0) + (min * 0.5) - 90.0;
    
    // Segundos (Linha fina longa)
    u8g2->drawLine(cx, cy, cx + (cos(aSeg * PI / 180.0) * 27), cy + (sin(aSeg * PI / 180.0) * 27));
    // Minutos
    u8g2->drawLine(cx, cy, cx + (cos(aMin * PI / 180.0) * 22), cy + (sin(aMin * PI / 180.0) * 22));
    // Horas (Curta)
    u8g2->drawLine(cx, cy, cx + (cos(aHor * PI / 180.0) * 15), cy + (sin(aHor * PI / 180.0) * 15));
}

void scWatchfacesM2::_desenharRelogioDigital() {
    U8G2* u8g2 = _display->getU8G2();
    u8g2->setFont(u8g2_font_logisoso32_tf); // Fonte grande 32px
    String horaStr = "--:--";
    if (_relogio && _relogio->estaSincronizado()) {
        uint32_t unixTime = _relogio->obterHoraUnix();
        int min = (unixTime % 3600) / 60;
        int hora = (unixTime % 86400) / 3600;
        char buf[6];
        sprintf(buf, "%02d:%02d", hora, min);
        horaStr = String(buf);
    }
    u8g2->drawStr(10, 45, horaStr.c_str()); 
}

void scWatchfacesM2::_desenharDashboard() {
    U8G2* u8g2 = _display->getU8G2();
    u8g2->setFont(u8g2_font_ncenB08_tr);
    
    u8g2->drawStr(0, 10, "CONSUMO GERAL:");
    
    u8g2->setFont(u8g2_font_ncenB14_tr);
    float totalKw = _reles ? (_reles->getConsumoKWh(1) + _reles->getConsumoKWh(2)) : 0.0;
    u8g2->setCursor(0, 30);
    u8g2->print(totalKw, 2);
    u8g2->print(" KWh");
    
    // Status das cargas
    u8g2->setFont(u8g2_font_ncenB08_tr);
    if (_reles) {
        u8g2->drawStr(0, 50, _reles->getEstadoRele(1) ? "R1: ON" : "R1: OFF");
        u8g2->drawStr(60, 50, _reles->getEstadoRele(2) ? "R2: ON" : "R2: OFF");
    }
}

void scWatchfacesM2::_desenharSimples() {
    U8G2* u8g2 = _display->getU8G2();
    
    // 1. Relogio Digital Grande centralizado
    u8g2->setFont(u8g2_font_logisoso32_tf); 
    String horaStr = "--:--";
    if (_relogio && _relogio->estaSincronizado()) {
        uint32_t unixTime = _relogio->obterHoraUnix();
        int min = (unixTime % 3600) / 60;
        int hora = (unixTime % 86400) / 3600;
        char buf[6];
        sprintf(buf, "%02d:%02d", hora, min);
        horaStr = String(buf);
    }
    // Desenha centralizado (X=20 aproxima pro meio em fonte 32)
    u8g2->drawStr(18, 38, horaStr.c_str()); 
    
    // 2. Rodapé com Informações Secundárias
    u8g2->setFont(u8g2_font_ncenB08_tr);
    
    // Consumo (Esquerda)
    float totalKw = _reles ? (_reles->getConsumoKWh(1) + _reles->getConsumoKWh(2)) : 0.0;
    char bufKw[16];
    sprintf(bufKw, "%.2f KWh", totalKw);
    u8g2->drawStr(0, 62, bufKw);
    
    // Temperatura M1 (Direita)
    float tempM1 = _telemetria ? _telemetria->obterTempM1() : 0.0;
    char bufTemp[16];
    if (tempM1 > 0.0) {
        sprintf(bufTemp, "%.1f C", tempM1);
    } else {
        sprintf(bufTemp, "-- C");
    }
    int w = u8g2->getStrWidth(bufTemp);
    u8g2->drawStr(128 - w, 62, bufTemp);
}

// ====================================================================
// CONTINGÊNCIA GÁS
// ====================================================================

void scWatchfacesM2::desenharAlertaGasTelaCheia() {
    if (!_display || !_display->getU8G2()) return; // Sem flood para redundancia, ja logado pelo watchface
    U8G2* u8g2 = _display->getU8G2();
    
    // Fundo Invertido (Branco)
    u8g2->setDrawColor(1);
    u8g2->drawBox(0, 0, 128, 64);
    
    // Texto Preto
    u8g2->setDrawColor(0);
    u8g2->setFont(u8g2_font_ncenB10_tr);
    u8g2->drawStr(10, 35, "PERIGO GÁS!");
    
    // Restaura cor para o loop
    u8g2->setDrawColor(1);
}

void scWatchfacesM2::desenharBannerGas() {
    if (!_display || !_display->getU8G2()) return;
    U8G2* u8g2 = _display->getU8G2();
    
    u8g2->setDrawColor(1);
    u8g2->drawBox(0, 54, 128, 10); // Banner de 10px no rodape
    u8g2->setDrawColor(0);
    u8g2->setFont(u8g2_font_5x7_tr);
    u8g2->drawStr(5, 62, "EMERGENCIA DE GAS ATIVA");
    u8g2->setDrawColor(1);
}

// ====================================================================
// MOTOR DO MENU EM LISTA
// ====================================================================

void scWatchfacesM2::desenharListaMenu(int nivelAtual, int cursor, bool emEdicao, bool m1Inativo, uint8_t releAlvo, int cursorEdicao) {
    if (!_display || !_display->getU8G2()) {
        static bool erroMenuLogado = false;
        if (_logger && !erroMenuLogado) {
            _logger->erro("WATCHFACES", "Ponteiro Grafico nulo durante loop de Menu!");
            erroMenuLogado = true;
        }
        return;
    }
    U8G2* u8g2 = _display->getU8G2();
    u8g2->setFont(u8g2_font_ncenB08_tr);
    
    if (nivelAtual == 7) { // 7 = MENU_SISTEMA_INF (Aba de Diagnóstico)
        _desenharSistemaInf();
        return;
    }
    
    if (nivelAtual == 6) { // 6 = MENU_AGENDAMENTOS
        u8g2->drawStr(0, 10, "> AGENDAMENTOS");
        u8g2->drawLine(0, 13, 128, 13);
        
        u8g2->drawStr(10, 30, "Acesse o");
        u8g2->drawStr(10, 45, "Painel WEB");
        
        u8g2->setDrawColor(1);
        u8g2->drawBox(0, 53, 128, 11);
        u8g2->setDrawColor(0);
        u8g2->drawStr(5, 62, "Voltar");
        u8g2->setDrawColor(1);
        return;
    }

    // Mock das strings do menu (Seria otimizado para PROGMEM na pratica)
    const char* itensRoot[] = {"Watchfaces", "Rele 1", "Rele 2", "Agendamentos", "Sistema (INF)", "Voltar"};
    const char* itensWf[] = {"Analogico", "Digital", "Dashboard", "Simples", "Desligar Tela", "Voltar"};
    const char* itensRelesSel[] = {"Selecionar Rele 1", "Selecionar Rele 2", "Voltar"};
    const char* itensAgendamento[] = {"Criar Novo", "Ver Salvos", "Apagar Todos", "Voltar"};
    
    // Arrays separados para evitar bug de indexação ao ocultar "Regra de Gas"
    const char* itensComGas[] = {"Ligar / Desligar", "Potencia (W)", "Regra de Gas", "Retorno Queda", "Resetar Consumo", "Voltar"};
    const char* itensSemGas[] = {"Ligar / Desligar", "Potencia (W)", "Retorno Queda", "Resetar Consumo", "Voltar"};
    const char* itensRegraGas[] = {"Ignorar", "Desligar / Bloquear", "Forcar Exaustor", "Voltar"};
    const char* itensRetornoQueda[] = {"Sempre Desligado", "Sempre Ligado", "Manter Ultimo", "Voltar"};
    
    const char** listaAtual = itensRoot;
    int maxItens = 6;
    
    if (nivelAtual == 1) { listaAtual = itensWf; maxItens = 6; }      // MENU_WATCHFACE_SEL
    if (nivelAtual == 2) { listaAtual = itensRelesSel; maxItens = 3;} // MENU_RELE_SELECIONAR
    if (nivelAtual == 3) {                                            // MENU_RELE_OPCOES
        listaAtual = m1Inativo ? itensSemGas : itensComGas;
        maxItens = m1Inativo ? 5 : 6; 
    }
    if (nivelAtual == 4) { listaAtual = itensRetornoQueda; maxItens = 4; } // MENU_RETORNO_QUEDA
    if (nivelAtual == 5) { listaAtual = itensRegraGas; maxItens = 4; } // MENU_REGRA_GAS

    // Calculo de Janela Visível (Paginação para não desenhar fora dos 64px de altura)
    int offset = 0;
    if (cursor > 3) offset = cursor - 3;
    
    u8g2->drawStr(0, 10, ">"); // Marcador simples para o Header
    u8g2->drawStr(10, 10, nivelAtual == 0 ? "MENU PRINCIPAL" : "OPCOES");
    u8g2->drawLine(0, 13, 128, 13);
    
    for (int i = 0; i < 4; i++) {
        int idxReal = i + offset;
        if (idxReal >= maxItens) break;
        
        int y = 26 + (i * 12);
        
        if (idxReal == cursor) { // Linha Selecionada
            u8g2->setDrawColor(1);
            u8g2->drawBox(0, y - 9, 128, 11);
            u8g2->setDrawColor(0); // Texto Invertido
        }
        
        u8g2->drawStr(5, y, listaAtual[idxReal]);
        
        // Se estivermos no modo edicao e esta é a linha do cursor, desenha o valor animado
        if (emEdicao && idxReal == cursor) {
            u8g2->drawStr(90, y, String(cursorEdicao).c_str());
        }
        
        u8g2->setDrawColor(1); // Volta pro padrao
    }
}

void scWatchfacesM2::_desenharSistemaInf() {
    U8G2* u8g2 = _display->getU8G2();
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(0, 10, "INF. DO SISTEMA");
    u8g2->drawLine(0, 12, 128, 12);
    
    if (_telemetria) {
        u8g2->drawStr(0, 25, ("IP: " + _telemetria->obterIP()).c_str());
        u8g2->drawStr(0, 38, ("MAC: " + _telemetria->obterMAC()).c_str());
        
        String logRede = "Rede: " + _telemetria->obterStatusRede() + " (" + String(_telemetria->obterRSSI()) + "dBm)";
        u8g2->drawStr(0, 51, logRede.c_str());
        
        u8g2->drawStr(0, 64, ("Temp M1: " + String(_telemetria->obterTempM1()) + "C").c_str());
    } else {
        u8g2->drawStr(0, 30, "Buscando Rede...");
    }
}
