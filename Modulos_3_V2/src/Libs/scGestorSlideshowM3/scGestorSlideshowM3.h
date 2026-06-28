#ifndef SC_GESTOR_SLIDESHOW_M3_H
#define SC_GESTOR_SLIDESHOW_M3_H

#include <Arduino.h>
#include <lvgl.h>
#include <SD.h>

class scLogger;

#define MAX_ARQUIVOS_IMAGEM 20
#define TAM_NOME_ARQUIVO 32
#define TEMPO_SLIDE_MS 10000 // 10 segundos por cada imagem RAW

typedef void (*SlideshowInterrompidoCb)();

class scGestorSlideshowM3 {
private:
    scLogger* _logger;
    bool _ativo;
    
    // Arrays estáticos para mapear caminhos de imagens sem alocar "String"
    char _arquivos[MAX_ARQUIVOS_IMAGEM][TAM_NOME_ARQUIVO];
    int _totalArquivos;
    int _indiceAtual;
    
    uint32_t _ultimoTroca;
    
    // Objetos LVGL de ancoragem
    lv_obj_t* _imgObj;
    lv_obj_t* _telaPai;
    
    SlideshowInterrompidoCb _cbInterrompido;

    void _escanearPastaImagens();
    void _carregarImagemAtual();

public:
    scGestorSlideshowM3();

    void inicializar(scLogger* logger);

    // Inicia a apresentação (Associa a imagem à tela atual do LVGL)
    void iniciar(lv_obj_t* telaPai, SlideshowInterrompidoCb cb);

    // Desliga a apresentação (Libera memória gráfica LVGL e retorna ao Dashboard)
    void parar();

    // Roda no Super Loop: Rotaciona imagens e monitora TouchScreen para desarmar
    void processar();
    
    bool isAtivo() const;
};

#endif // SC_GESTOR_SLIDESHOW_M3_H
