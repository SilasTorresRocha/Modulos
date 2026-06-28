#ifndef SC_WATCHFACES_M3_H
#define SC_WATCHFACES_M3_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "../../LibsGlobais/scLogger/scLogger.h"

// Dimensões do ILI9488 em modo paisagem
#define TELA_LARGURA 480
#define TELA_ALTURA  320

class scWatchfacesM3 {
public:
    scWatchfacesM3();

    // Inicializa a camada grafica LVGL + TFT_eSPI (incluindo setup de DMA)
    void inicializar(scLogger* logger);

    // Deve ser chamada continuamente dentro da Task_AtualizarTela do FreeRTOS
    void atualizar();

    // Setters assíncronos para atualizar a UI a partir da Task de Rede
    void atualizarStatusWiFi(bool conectado);
    void atualizarStatusGas(bool perigo);
    void atualizarTemperatura(float temp);

private:
    scLogger* _logger;
    TFT_eSPI _tft;

    // Elementos da UI
    lv_obj_t* _labelTemperatura;
    lv_obj_t* _labelWifi;
    lv_obj_t* _painelGas;
    
    // Configura a UI Inicial
    void desenharDashboard();
};

#endif // SC_WATCHFACES_M3_H
