#ifndef SC_WATCHFACES_M2_H
#define SC_WATCHFACES_M2_H

#include <Arduino.h>
#include "../../LibsGlobais/scGestorDisplay/scGestorDisplay.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "../scGestorReles/scGestorReles.h"
#include "../scTelemetriaM2/scTelemetriaM2.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

class scWatchfacesM2 {
public:
    scWatchfacesM2();
    void inicializar(scLogger* logger, scGestorDisplay* display, scRelogioSincronizado* relogio, scGestorReles* reles, scTelemetriaM2* telemetria);
    
    void desenharWatchface(uint8_t id_watchface);
    
    // Alertas de Contingencia
    void desenharAlertaGasTelaCheia();
    void desenharBannerGas();
    
    // Core do Menu
    void desenharListaMenu(int nivelAtual, int cursor, bool emEdicao, bool m1Inativo, uint8_t releAlvo, int cursorEdicao);
    
private:
    scLogger* _logger;
    scGestorDisplay* _display;
    scRelogioSincronizado* _relogio;
    scGestorReles* _reles;
    scTelemetriaM2* _telemetria;
    
    // Primitivas Visuais Ociosas
    void _desenharRelogioAnalogico();
    void _desenharRelogioDigital();
    void _desenharDashboard();
    void _desenharSimples();
    
    // Primitivas do Menu
    void _desenharSistemaInf();
};

#endif // SC_WATCHFACES_M2_H
