#include "scWatchfacesM1.h"

// Mantém a tela fluida mas sem roubar ciclos excessivos de I2C/CPU
#define FPS_INTERVALO_MS 200 

scWatchfacesM1::scWatchfacesM1() {
    _logger = nullptr;
    _display = nullptr;
    _relogio = nullptr;
    _gas = nullptr;
    _termico = nullptr;
    _modoTelaBase = 2; // Inicia na Completinha
    _ultimoFrame = 0;
}

void scWatchfacesM1::inicializar(scLogger* logger, scGestorDisplay* display, scRelogioSincronizado* relogio, scMonitorGasM1* gas, scLeitorTermicoM1* termico) {
    _logger = logger;
    _display = display;
    _relogio = relogio;
    _gas = gas;
    _termico = termico;

    // Princípio 9: Loga alto e claro se faltar o motor físico gráfico
    if (_logger && (!_display || !_display->getU8G2())) {
        _logger->erro("WATCH_M1", "Falha Critica: O Motor de Render U8G2 nao foi injetado!");
    }
}

void scWatchfacesM1::setModoTela(uint8_t modoTela) {
    if (_modoTelaBase != modoTela) {
        _modoTelaBase = modoTela;
        if (_logger) {
            _logger->info("WATCH_M1", "Modo UI alterado via Nuvem/Hub para o layout: " + String(_modoTelaBase));
        }
    }
}

void scWatchfacesM1::atualizar() {
    if (!_display || !_display->getU8G2()) return;
    
    // Limitador de FPS passivo (non-blocking) para folgar as lógicas de saúde
    if (millis() - _ultimoFrame < FPS_INTERVALO_MS) {
        return;
    }
    _ultimoFrame = millis();
    
    U8G2* u8g2 = _display->getU8G2();
    
    // -------------------------------------------------------------
    // INTELIGÊNCIA DE WAKE-UP (OVERRIDE DE SEGURANÇA)
    // -------------------------------------------------------------
    bool perigoGas = (_gas && _gas->VazamentoDetectado());
    bool fornoQuente = (_termico && _termico->FornoLigado());
    
    uint8_t modoEfetivo = _modoTelaBase;
    
    // Se a tela estava mandada ficar preta (Modo 1), o Módulo acorda ela à força!
    if (perigoGas) {
        modoEfetivo = 99; // ID interno arbitrário para a tela de Perigo Máximo
    } else if (fornoQuente && modoEfetivo == 1) {
        modoEfetivo = 3;  // Acorda no modo Essencial para mostrar que não esfriou ainda
    }

    // -------------------------------------------------------------
    // RENDERIZAÇÃO NO BUFFER C++
    // -------------------------------------------------------------
    u8g2->clearBuffer();
    
    switch (modoEfetivo) {
        case 1: 
            // Modo 1 = Sleep. O Buffer fica preto.
            break; 
        case 2:
            _desenharCompleto();
            break;
        case 3:
            _desenharEssencial();
            break;
        case 99:
            _desenharAlertaEmergencia();
            break;
        default:
            _desenharCompleto();
            break;
    }
    
    // -------------------------------------------------------------
    // FLUSH PARA O DISPLAY FÍSICO I2C
    // -------------------------------------------------------------
    u8g2->sendBuffer();
}

void scWatchfacesM1::_desenharCompleto() {
    U8G2* u8g2 = _display->getU8G2();
    
    // --- STATUS BAR (TOP) ---
    String horaStr = "--:--";
    if (_relogio && _relogio->estaSincronizado()) {
        uint32_t unixTime = _relogio->obterHoraUnix();
        int min = (unixTime % 3600) / 60;
        int hora = (unixTime % 86400) / 3600;
        char buf[6];
        sprintf(buf, "%02d:%02d", hora, min);
        horaStr = String(buf);
    }
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(0, 10, horaStr.c_str());
    
    // Linha separadora
    u8g2->drawLine(0, 12, 128, 12);
    
    // --- BURACO NEGRO DO FORNO ---
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(0, 27, "Forno:");
    
    u8g2->setFont(u8g2_font_ncenB14_tr);
    if (_termico) {
        u8g2->setCursor(45, 29);
        u8g2->print(_termico->obterTemperatura(), 1);
        u8g2->print(" C");
    }
    
    // --- ESTADO DO GÁS ---
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(0, 46, "Gás (ADC):");
    if (_gas) {
        u8g2->setCursor(65, 46);
        u8g2->print(_gas->obterNivelGas());
    }
    
    // --- STATUS GERAL ---
    u8g2->setFont(u8g2_font_4x6_tr);
    if (_termico && _termico->FornoLigado()) {
        u8g2->drawStr(0, 62, "MOTOR TERMICO: LIGADO (AQUECENDO)");
    } else {
        u8g2->drawStr(0, 62, "MOTOR TERMICO: EM REPOUSO");
    }
}

void scWatchfacesM1::_desenharEssencial() {
    U8G2* u8g2 = _display->getU8G2();
    
    if (_termico) {
        u8g2->setFont(u8g2_font_logisoso32_tf); // Fonte Brutalista
        String temp = String((int)_termico->obterTemperatura()) + "C";
        
        // Centraliza grosseiramente
        u8g2->drawStr(10, 45, temp.c_str());
        
        u8g2->setFont(u8g2_font_ncenB10_tr);
        if (_termico->FornoLigado()) {
            u8g2->drawStr(15, 62, "CHAMA ATIVA");
        } else {
            u8g2->drawStr(15, 62, "EM REPOUSO");
        }
    }
}

void scWatchfacesM1::_desenharAlertaEmergencia() {
    U8G2* u8g2 = _display->getU8G2();
    
    // Calcula um piscar a cada 500ms baseado no loop C++ (sem bloquear)
    bool inverterCores = (millis() % 1000 < 500);
    
    if (inverterCores) {
        u8g2->setDrawColor(1); // Acende todos os pixels (Tela inteira Branca)
        u8g2->drawBox(0, 0, 128, 64);
        u8g2->setDrawColor(0); // Seta tinta para Preto
    } else {
        // Fundo preto normal (Já garantido pelo clearBuffer)
        u8g2->setDrawColor(1); // Seta tinta para Branco
    }
    
    u8g2->setFont(u8g2_font_ncenB14_tr);
    u8g2->drawStr(5, 30, "PERIGO GAS!");
    
    u8g2->setFont(u8g2_font_ncenB08_tr);
    u8g2->drawStr(10, 50, "RISCO DE EXPLOSAO");
    
    // Restaura a cor de fábrica do render para não bugar o próximo frame
    u8g2->setDrawColor(1); 
}
