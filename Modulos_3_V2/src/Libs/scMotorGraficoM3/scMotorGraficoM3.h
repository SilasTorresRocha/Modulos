#ifndef SC_MOTOR_GRAFICO_M3_H
#define SC_MOTOR_GRAFICO_M3_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <SD.h>

class scLogger;
class scMonitorSaudeM3;

// Resoluções padroes do ILI9488
#define TFT_LARGURA 480
#define TFT_ALTURA 320

// Define o pino de controle de Backlight via PWM 
#define PINO_BACKLIGHT 32
#define CANAL_PWM_BKL 0

// Regras de Zero Heap Allocation
#define MAX_ARQUIVOS_ABERTOS_LVGL 4

class scMotorGraficoM3 {
private:
    scLogger* _logger;
    scMonitorSaudeM3* _saude;
    TFT_eSPI _tft;

    // Buffers de Vídeo Alocados no Heap (DMA Capable) para não estourar a secão .bss (DRAM0)
    lv_color_t* _bufDraw;
    lv_disp_draw_buf_t _drawBufStruct;
    lv_disp_drv_t _dispDrv;
    lv_indev_drv_t _indevDrv;
    lv_fs_drv_t _fsDrv;

    bool _ativo;
    uint32_t _ultimoLoopLvgl;

    // Métodos Ponte C para os Callbacks nativos do LVGL (Hardware Render & Input)
    static void _tftFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p);
    static void _touchReadCb(lv_indev_drv_t* indev_driver, lv_indev_data_t* data);

    // Callbacks Ponte C para o LVGL File System (Permite leitura de .bin direto do SD Card)
    static void* _fsOpenCb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode);
    static lv_fs_res_t _fsCloseCb(lv_fs_drv_t * drv, void * file_p);
    static lv_fs_res_t _fsReadCb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br);
    static lv_fs_res_t _fsSeekCb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence);
    static lv_fs_res_t _fsTellCb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p);

public:
    scMotorGraficoM3();

    // Inicializa a camada física SPI do LCD e o Core semântico do LVGL
    bool inicializar(scLogger* logger, scMonitorSaudeM3* saude);

    // Bate a engrenagem LVGL. Deve estar estritamente no Super Loop para isolar SPI de rede/SD
    void processar();

    // Utilitários de Hardware Fisico (Backlight / Inversão de Tela)
    void ajustarBrilho(uint8_t brilho);
    void rotacionar(uint8_t orientacao);
    
    // Retorna a estância raiz para módulos avançados pintarem direto se necessário
    TFT_eSPI* obterDriverFisico();
    
    bool isAtivo() const;
};

#endif // SC_MOTOR_GRAFICO_M3_H
