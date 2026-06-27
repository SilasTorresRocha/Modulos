#ifndef SC_WATCHFACES_M1_H
#define SC_WATCHFACES_M1_H

#include <Arduino.h>
#include "../../LibsGlobais/scGestorDisplay/scGestorDisplay.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

// Injeção de Negócio (Módulo 1)
#include "../scMonitorGasM1/scMonitorGasM1.h"
#include "../scLeitorTermicoM1/scLeitorTermicoM1.h"

class scWatchfacesM1 {
public:
    scWatchfacesM1();
    
    // Inicialização mandatória agregando todas as bibliotecas de input visual
    void inicializar(scLogger* logger, scGestorDisplay* display, scRelogioSincronizado* relogio, scMonitorGasM1* gas, scLeitorTermicoM1* termico);
    
    // Atualiza a preferência remota vinda do Backend/Hub (tela_idle)
    void setModoTela(uint8_t modoTela);
    
    // Motor gráfico passivo. Deve rodar no super loop.
    void atualizar();
    
private:
    scLogger* _logger;
    scGestorDisplay* _display;
    scRelogioSincronizado* _relogio;
    scMonitorGasM1* _gas;
    scLeitorTermicoM1* _termico;
    
    uint8_t _modoTelaBase; 
    uint32_t _ultimoFrame; // Limitador de FPS
    
    // Primitivas Visuais
    void _desenharCompleto();
    void _desenharEssencial();
    void _desenharAlertaEmergencia();
};

#endif // SC_WATCHFACES_M1_H
