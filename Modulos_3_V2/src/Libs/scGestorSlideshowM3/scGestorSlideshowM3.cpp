#include "scGestorSlideshowM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

scGestorSlideshowM3::scGestorSlideshowM3() : 
    _logger(nullptr), _ativo(false), _totalArquivos(0), _indiceAtual(0), 
    _ultimoTroca(0), _imgObj(nullptr), _telaPai(nullptr), _cbInterrompido(nullptr) {
}

void scGestorSlideshowM3::inicializar(scLogger* logger) {
    _logger = logger;
    
    // Faz a varredura primária na raiz do SD atras da pasta /Imagens
    _escanearPastaImagens();

    if (_logger != nullptr) {
        char buf[90];
        snprintf(buf, sizeof(buf), "Gestor de Slideshow montado. (%d) Imagens RAW cadastradas para Descanso.", _totalArquivos);
        _logger->info("scGestorSlideshow", buf);
    }
}

void scGestorSlideshowM3::_escanearPastaImagens() {
    _totalArquivos = 0;
    
    File dir = SD.open("/Imagens");
    if (!dir || !dir.isDirectory()) {
        if (_logger != nullptr) _logger->warn("scGestorSlideshow", "Pasta /Imagens nao encontrada no SD Card. Slideshow passivo cancelado.");
        return;
    }

    File file = dir.openNextFile();
    while (file && _totalArquivos < MAX_ARQUIVOS_IMAGEM) {
        if (!file.isDirectory()) {
            const char* nome = file.name();
            // Verifica integridade da extensao (.bin) de LVGL (Raw Pixels)
            const char* ext = strrchr(nome, '.');
            if (ext != nullptr && strcmp(ext, ".bin") == 0) {
                snprintf(_arquivos[_totalArquivos], TAM_NOME_ARQUIVO, "%s", nome);
                _totalArquivos++;
            }
        }
        file.close(); // Essencial no Bare-Metal para liberar os ponteiros do FS do ESP32
        file = dir.openNextFile();
    }
}

void scGestorSlideshowM3::iniciar(lv_obj_t* telaPai, SlideshowInterrompidoCb cb) {
    if (_totalArquivos == 0 || telaPai == nullptr) return;

    _telaPai = telaPai;
    _cbInterrompido = cb;
    _ativo = true;
    _indiceAtual = 0;
    
    // Resseta o inatividade global do display para evitar loop instantâneo
    lv_disp_trig_activity(lv_disp_get_default());
    _ultimoTroca = millis();

    // Cria o Widget nativo da imagem atrelado à UI do LVGL
    _imgObj = lv_img_create(_telaPai);
    lv_obj_center(_imgObj);
    
    _carregarImagemAtual();
}

void scGestorSlideshowM3::parar() {
    if (!_ativo) return;
    
    _ativo = false;
    
    // Apaga a arvore binaria grafica para devolver os 30KB de Video RAM
    if (_imgObj != nullptr) {
        lv_obj_del(_imgObj);
        _imgObj = nullptr;
    }

    if (_logger != nullptr) _logger->info("scGestorSlideshow", "Slideshow desativado.");
}

void scGestorSlideshowM3::_carregarImagemAtual() {
    if (_imgObj == nullptr || _totalArquivos == 0) return;

    // A magica do FS Driver customizado do LVGL (scMotorGraficoM3) operando.
    // O prefixo "S:" roteia o ponteiro para os blocos fisicos da placa SD.
    char srcPath[64];
    snprintf(srcPath, sizeof(srcPath), "S:Imagens/%s", _arquivos[_indiceAtual]);

    lv_img_set_src(_imgObj, srcPath);
}

void scGestorSlideshowM3::processar() {
    if (!_ativo || _totalArquivos == 0) return;

    // 1. Deteccao Autônoma de Toque
    uint32_t tempoInativo = lv_disp_get_inactive_time(lv_disp_get_default());
    if (tempoInativo < 100) { 
        // O usuario botou o dedo na tela!
        parar();
        
        // Aciona o Cérebro de Telas para restaurar o Dashboard Imediatamente
        if (_cbInterrompido != nullptr) {
            _cbInterrompido();
        }
        return;
    }

    // 2. Rotatividade do Álbum
    uint32_t agora = millis();
    if (agora - _ultimoTroca >= TEMPO_SLIDE_MS) {
        _ultimoTroca = agora;
        
        _indiceAtual = (_indiceAtual + 1) % _totalArquivos;
        _carregarImagemAtual(); // O arquivo flui limpo via FS SPI driver atómico
    }
}

bool scGestorSlideshowM3::isAtivo() const {
    return _ativo;
}
