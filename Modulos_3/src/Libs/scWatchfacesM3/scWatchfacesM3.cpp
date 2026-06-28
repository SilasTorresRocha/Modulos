#include "scWatchfacesM3.h"

// Instância global do TFT necessária para o callback estático do LVGL
static TFT_eSPI tftGlobal = TFT_eSPI();

/* Callback de DMA/Flush exigido pelo LVGL */
static void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tftGlobal.startWrite();
    tftGlobal.setAddrWindow(area->x1, area->y1, w, h);
    
    // Fallback: ILI9488 nao suporta DMA nativo de 16-bits na TFT_eSPI
    tftGlobal.pushColors((uint16_t *)&color_p->full, w * h, true);
    tftGlobal.endWrite();

    lv_disp_flush_ready(disp_drv);
}

/* Callback de Touch (Opcional, caso use o touch do TFT_eSPI) */
static void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint16_t touchX, touchY;

    // getTouch exige configuração no User_Setup.h do TFT_eSPI
    bool touched = tftGlobal.getTouch(&touchX, &touchY);

    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

scWatchfacesM3::scWatchfacesM3() {
    _logger = nullptr;
    _labelTemperatura = nullptr;
    _labelWifi = nullptr;
    _painelGas = nullptr;
}

void scWatchfacesM3::inicializar(scLogger* logger) {
    _logger = logger;
    
    _logger->info("UI_M3", "Iniciando Motor Grafico TFT_eSPI com DMA...");
    
    tftGlobal.begin();
    tftGlobal.setRotation(1); // Paisagem (480x320)
    
    // Nota: O ILI9488 so suporta DMA em modo 18-bits (RGB666) pela TFT_eSPI, entao 
    // desabilitamos o initDMA() para evitar linker errors, usando SRAM padrao.
    // Calibração de touch dummy (necessário calibrar na prática)
    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
    tftGlobal.setTouch(calData);

    _logger->info("UI_M3", "Iniciando nucleo LVGL...");
    lv_init();

    // Alocação de memória DMA para o Buffer do LVGL
    // Para DMA, a memoria DEVE ser alocada usando propriedades especificas do ESP32
    size_t buffer_size = TELA_LARGURA * 40; // ~40 linhas = ~38KB (cabe na SRAM do S2 tranquilamente)
    lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(buffer_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    
    if (!buf1) {
        _logger->erro("UI_M3", "Falha critica ao alocar memoria DMA para o LVGL!");
        return; // Crash evitavel
    }

    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, buffer_size);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = TELA_LARGURA;
    disp_drv.ver_res = TELA_ALTURA;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    desenharDashboard();
    
    _logger->info("UI_M3", "LVGL montado e pronto para renderizacao!");
}

void scWatchfacesM3::desenharDashboard() {
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x1a1a1a), LV_PART_MAIN);

    // Barra de cabecalho
    lv_obj_t* header = lv_obj_create(lv_scr_act());
    lv_obj_set_size(header, TELA_LARGURA, 40);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);

    // Titulo
    lv_obj_t* titulo = lv_label_create(header);
    lv_label_set_text(titulo, "HUB CENTRAL");
    lv_obj_align(titulo, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_text_color(titulo, lv_color_hex(0xFFFFFF), LV_PART_MAIN);

    // Wi-Fi Label
    _labelWifi = lv_label_create(header);
    lv_label_set_text(_labelWifi, "WIFI: OFF");
    lv_obj_align(_labelWifi, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_set_style_text_color(_labelWifi, lv_color_hex(0xFF0000), LV_PART_MAIN);

    // Card de Temperatura RTC
    lv_obj_t* cardTemp = lv_obj_create(lv_scr_act());
    lv_obj_set_size(cardTemp, 200, 100);
    lv_obj_align(cardTemp, LV_ALIGN_CENTER, -110, -30);
    
    lv_obj_t* tempTitle = lv_label_create(cardTemp);
    lv_label_set_text(tempTitle, "Temp. Ambiente");
    lv_obj_align(tempTitle, LV_ALIGN_TOP_MID, 0, 0);
    
    _labelTemperatura = lv_label_create(cardTemp);
    lv_label_set_text(_labelTemperatura, "-- C");
    lv_obj_align(_labelTemperatura, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_text_color(_labelTemperatura, lv_color_hex(0x00FF00), LV_PART_MAIN);

    // Card de Gas / Emergencia
    _painelGas = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_painelGas, 200, 100);
    lv_obj_align(_painelGas, LV_ALIGN_CENTER, 110, -30);
    lv_obj_set_style_bg_color(_painelGas, lv_color_hex(0x004400), LV_PART_MAIN);
    
    lv_obj_t* gasTitle = lv_label_create(_painelGas);
    lv_label_set_text(gasTitle, "Status Gas");
    lv_obj_align(gasTitle, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(gasTitle, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
}

void scWatchfacesM3::atualizarStatusWiFi(bool conectado) {
    if (_labelWifi) {
        lv_label_set_text(_labelWifi, conectado ? "WIFI: ON" : "WIFI: OFF");
        lv_obj_set_style_text_color(_labelWifi, conectado ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), LV_PART_MAIN);
    }
}

void scWatchfacesM3::atualizarStatusGas(bool perigo) {
    if (_painelGas) {
        lv_obj_set_style_bg_color(_painelGas, perigo ? lv_color_hex(0xFF0000) : lv_color_hex(0x004400), LV_PART_MAIN);
        lv_obj_t* title = lv_obj_get_child(_painelGas, 0);
        if (title) {
            lv_label_set_text(title, perigo ? "VAZAMENTO!!" : "Seguro");
        }
    }
}

void scWatchfacesM3::atualizarTemperatura(float temp) {
    if (_labelTemperatura) {
        String t = String(temp, 1) + " C";
        lv_label_set_text(_labelTemperatura, t.c_str());
    }
}

void scWatchfacesM3::atualizar() {
    // Roda a engrenagem interna do LVGL
    lv_timer_handler();
}
