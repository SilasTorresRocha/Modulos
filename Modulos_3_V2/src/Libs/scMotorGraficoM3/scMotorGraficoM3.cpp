#include "scMotorGraficoM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scMonitorSaudeM3/scMonitorSaudeM3.h"

// Truque C/C++ global para callbacks estáticos do LVGL acessarem o hardware SPI atrelado a este escopo
static TFT_eSPI* ptrHardwareTft = nullptr;

// Pool de Arquivos Estáticos (Zero Alocação Dinâmica "new" para imagens do SD)
static File _fsFilesPool[MAX_ARQUIVOS_ABERTOS_LVGL];
static bool _fsOcupadoPool[MAX_ARQUIVOS_ABERTOS_LVGL];

scMotorGraficoM3::scMotorGraficoM3() : _logger(nullptr), _saude(nullptr), _tft(TFT_eSPI()), _ativo(false), _ultimoLoopLvgl(0) {
    for (int i = 0; i < MAX_ARQUIVOS_ABERTOS_LVGL; i++) {
        _fsOcupadoPool[i] = false;
    }
}

bool scMotorGraficoM3::inicializar(scLogger* logger, scMonitorSaudeM3* saude) {
    _logger = logger;
    _saude = saude;
    ptrHardwareTft = &_tft;

    // 1. Setup Básico do LCD via SPI
    _tft.begin();
    _tft.setRotation(1); // Paisagem default (480x320)
    
    // Calibração embutida (Valores dependem do display XPT2046)
    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 }; 
    _tft.setTouch(calData);

    // 2. Setup Backlight Analógico (API Core v3.0+)
    ledcAttach(PINO_BACKLIGHT, 5000, 8);
    ajustarBrilho(255); // 100%

    // 3. Inicializa Core Semântico
    lv_init();

    // 4. Inicializa o Buffer de Rasterização na Stack/Heap Dinâmico (Para salvar o Linker)
    _bufDraw = (lv_color_t*)heap_caps_malloc(TFT_LARGURA * 30 * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!_bufDraw) {
        if (_logger != nullptr) _logger->erro("scMotorGrafico", "PANIC: Falha ao alocar Memoria DMA para Display!");
        return false;
    }
    lv_disp_draw_buf_init(&_drawBufStruct, _bufDraw, NULL, TFT_LARGURA * 30);

    // 5. Acopla LVGL ao Display de Saida (Renderer)
    lv_disp_drv_init(&_dispDrv);
    _dispDrv.hor_res = TFT_LARGURA;
    _dispDrv.ver_res = TFT_ALTURA;
    _dispDrv.flush_cb = _tftFlushCb;
    _dispDrv.draw_buf = &_drawBufStruct;
    lv_disp_drv_register(&_dispDrv);

    // 6. Acopla LVGL ao Painel de Toque (Input)
    lv_indev_drv_init(&_indevDrv);
    _indevDrv.type = LV_INDEV_TYPE_POINTER;
    _indevDrv.read_cb = _touchReadCb;
    lv_indev_drv_register(&_indevDrv);

    // 7. Configura Driver Lógico de Arquivos "S" (SD Card) - Ex: "S:imagem.bin"
    lv_fs_drv_init(&_fsDrv);
    _fsDrv.letter = 'S'; 
    _fsDrv.open_cb = _fsOpenCb;
    _fsDrv.close_cb = _fsCloseCb;
    _fsDrv.read_cb = _fsReadCb;
    _fsDrv.seek_cb = _fsSeekCb;
    _fsDrv.tell_cb = _fsTellCb;
    lv_fs_drv_register(&_fsDrv);

    _ativo = true;
    _ultimoLoopLvgl = millis();

    if (_logger != nullptr) {
        _logger->info("scMotorGrafico", "Placa de Video ativada. Hardware (ILI9488 + LVGL 8). File System acoplado (S:).");
    }

    return true;
}

void scMotorGraficoM3::processar() {
    if (!_ativo) return;

    // Cronometro fixo para evitar travamento da CPU 
    uint32_t agora = millis();
    if (agora - _ultimoLoopLvgl >= 5) { // 5ms loop cap (~200 fps logico)
        
        // Cão de Guarda da GPU: Se o flush demorar d+, o Monitor berra
        if (_saude != nullptr) {
            _saude->iniciarMedicaoLoopGrafico();
        }

        lv_timer_handler(); // Executa tarefas visuais pesadas do LVGL
        
        if (_saude != nullptr) {
            _saude->finalizarMedicaoLoopGrafico();
        }
        
        _ultimoLoopLvgl = agora;
    }
}

void scMotorGraficoM3::ajustarBrilho(uint8_t brilho) {
    ledcWrite(PINO_BACKLIGHT, brilho); // API Core v3.0+ usa pino direto, não canal
}

void scMotorGraficoM3::rotacionar(uint8_t orientacao) {
    _tft.setRotation(orientacao);
    
    // Alinhamento dinâmico sem rebotar
    if (orientacao % 2 == 0) {
        _dispDrv.hor_res = TFT_ALTURA;
        _dispDrv.ver_res = TFT_LARGURA;
    } else {
        _dispDrv.hor_res = TFT_LARGURA;
        _dispDrv.ver_res = TFT_ALTURA;
    }
    
    lv_disp_drv_update(lv_disp_get_default(), &_dispDrv);
    
    if (_logger != nullptr) _logger->info("scMotorGrafico", "Giroscopio logico acionado (TFT Rotacionado).");
}

TFT_eSPI* scMotorGraficoM3::obterDriverFisico() {
    return &_tft;
}

bool scMotorGraficoM3::isAtivo() const {
    return _ativo;
}


// =========================================================================
// PONTE FISICA (HARDWARE <-> LVGL)
// =========================================================================

void scMotorGraficoM3::_tftFlushCb(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p) {
    if (ptrHardwareTft == nullptr) return;

    uint32_t larguraBox = (area->x2 - area->x1 + 1);
    uint32_t alturaBox  = (area->y2 - area->y1 + 1);

    // Trava de Barramento (Isolamento SPI para proteger o Cartao SD)
    ptrHardwareTft->startWrite(); 
    ptrHardwareTft->setAddrWindow(area->x1, area->y1, larguraBox, alturaBox);
    ptrHardwareTft->pushColors((uint16_t*)&color_p->full, larguraBox * alturaBox, true);
    ptrHardwareTft->endWrite(); 

    lv_disp_flush_ready(disp);
}

void scMotorGraficoM3::_touchReadCb(lv_indev_drv_t* indev_driver, lv_indev_data_t* data) {
    if (ptrHardwareTft == nullptr) return;

    uint16_t touchX = 0, touchY = 0;
    bool tocado = ptrHardwareTft->getTouch(&touchX, &touchY);

    if (!tocado) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

// =========================================================================
// PONTE DE ARQUIVOS (FILE SYSTEM <-> LVGL)
// Objetivo: Carregar "S:logo.bin" do SD direto pra tela
// =========================================================================

void* scMotorGraficoM3::_fsOpenCb(lv_fs_drv_t * drv, const char * path, lv_fs_mode_t mode) {
    int slotLivre = -1;
    for (int i = 0; i < MAX_ARQUIVOS_ABERTOS_LVGL; i++) {
        if (!_fsOcupadoPool[i]) {
            slotLivre = i;
            break;
        }
    }
    
    if (slotLivre == -1) return NULL; // Panico: Pool Estourado

    char caminhoSD[64];
    snprintf(caminhoSD, sizeof(caminhoSD), "/%s", path); // Raiz do SD
    
    const char* flags = FILE_READ; // Somente leitura para interface UI
    
    File f = SD.open(caminhoSD, flags);
    if (!f) return NULL;

    _fsFilesPool[slotLivre] = f;
    _fsOcupadoPool[slotLivre] = true;
    
    return (void*)(uintptr_t)slotLivre; // Passa a Chave/Index em vez do ponteiro
}

lv_fs_res_t scMotorGraficoM3::_fsCloseCb(lv_fs_drv_t * drv, void * file_p) {
    int slot = (int)(uintptr_t)file_p;
    if (slot >= 0 && slot < MAX_ARQUIVOS_ABERTOS_LVGL) {
        _fsFilesPool[slot].close();
        _fsOcupadoPool[slot] = false;
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}

lv_fs_res_t scMotorGraficoM3::_fsReadCb(lv_fs_drv_t * drv, void * file_p, void * buf, uint32_t btr, uint32_t * br) {
    int slot = (int)(uintptr_t)file_p;
    if (slot >= 0 && slot < MAX_ARQUIVOS_ABERTOS_LVGL) {
        *br = _fsFilesPool[slot].read((uint8_t *)buf, btr);
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}

lv_fs_res_t scMotorGraficoM3::_fsSeekCb(lv_fs_drv_t * drv, void * file_p, uint32_t pos, lv_fs_whence_t whence) {
    int slot = (int)(uintptr_t)file_p;
    if (slot >= 0 && slot < MAX_ARQUIVOS_ABERTOS_LVGL) {
        uint32_t novaPos = pos;
        if (whence == LV_FS_SEEK_CUR) {
            novaPos = _fsFilesPool[slot].position() + pos;
        } else if (whence == LV_FS_SEEK_END) {
            novaPos = _fsFilesPool[slot].size() + pos;
        }
        
        _fsFilesPool[slot].seek(novaPos);
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}

lv_fs_res_t scMotorGraficoM3::_fsTellCb(lv_fs_drv_t * drv, void * file_p, uint32_t * pos_p) {
    int slot = (int)(uintptr_t)file_p;
    if (slot >= 0 && slot < MAX_ARQUIVOS_ABERTOS_LVGL) {
        *pos_p = _fsFilesPool[slot].position();
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}
