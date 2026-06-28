#include "scGestorTelasM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scRadarEcossistemaM3/scRadarEcossistemaM3.h"
#include "../scGestorDispositivosM3/scGestorDispositivosM3.h"

// Estilos Globais Estaticos do Motor C (Zero Heap Fragmentation na execucao)
static lv_style_t _estiloCard;
static lv_style_t _estiloBotaoAzul;
static lv_style_t _estiloAlerta;
static lv_style_t _estiloFontePequena;

scGestorTelasM3::scGestorTelasM3() : 
    _logger(nullptr), _radar(nullptr), _gestorDispositivos(nullptr),
    _telaDashboard(nullptr), _telaMenu(nullptr), _telaAlerta(nullptr),
    _labelAvisoM4(nullptr), _btnMenuM4(nullptr) {
    
    for (int i = 0; i < MAX_LOGS_UI; i++) {
        _labelsLog[i] = nullptr;
        memset(_historicoLogs[i], 0, TAM_LOG_UI);
    }
}

void scGestorTelasM3::inicializar(scLogger* logger, scRadarEcossistemaM3* radar, scGestorDispositivosM3* disp) {
    _logger = logger;
    _radar = radar;
    _gestorDispositivos = disp;

    // Inicializa a paleta uma unica vez
    _construirEstilosPadrao();

    // Instancia as telas-base (Isso garante 100% de estabilidade ao longo do uptime)
    _telaDashboard = lv_obj_create(NULL);
    _telaMenu = lv_obj_create(NULL);
    _telaAlerta = lv_obj_create(NULL);

    _construirDashboard();
    _construirMenu();
    _construirAlerta();

    mostrarDashboard();

    if (_logger != nullptr) _logger->info("scGestorTelas", "UI LVGL pre-alocada e estavel (Arquitetura C-Puro).");
}

void scGestorTelasM3::_construirEstilosPadrao() {
    lv_style_init(&_estiloCard);
    lv_style_set_bg_color(&_estiloCard, lv_color_hex(0x202020));
    lv_style_set_radius(&_estiloCard, 10);
    lv_style_set_border_width(&_estiloCard, 0);

    lv_style_init(&_estiloBotaoAzul);
    lv_style_set_bg_color(&_estiloBotaoAzul, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&_estiloBotaoAzul, 5);
    lv_style_set_text_color(&_estiloBotaoAzul, lv_color_white());

    lv_style_init(&_estiloAlerta);
    lv_style_set_bg_color(&_estiloAlerta, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_text_color(&_estiloAlerta, lv_color_white());
    
    lv_style_init(&_estiloFontePequena);
    lv_style_set_text_color(&_estiloFontePequena, lv_color_hex(0xAAAAAA));
}

void scGestorTelasM3::_construirDashboard() {
    lv_obj_set_style_bg_color(_telaDashboard, lv_color_black(), 0);

    lv_obj_t* labelTitulo = lv_label_create(_telaDashboard);
    lv_label_set_text(labelTitulo, "Painel de Controle M3 (Hub)");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    lv_obj_t* btn = lv_btn_create(_telaDashboard);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(btn, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btn, _btnMenuClickCb, LV_EVENT_CLICKED, this);
    
    lv_obj_t* btnLabel = lv_label_create(btn);
    lv_label_set_text(btnLabel, "Configuracoes");
}

void scGestorTelasM3::_construirMenu() {
    lv_obj_set_style_bg_color(_telaMenu, lv_color_black(), 0);

    // ==========================================
    // ABAS 1: Container do Ring Buffer de Logs
    // ==========================================
    lv_obj_t* containerLogs = lv_obj_create(_telaMenu);
    lv_obj_set_size(containerLogs, 220, 180);
    lv_obj_align(containerLogs, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_add_style(containerLogs, &_estiloCard, 0);
    lv_obj_set_layout(containerLogs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(containerLogs, LV_FLEX_FLOW_COLUMN);
    
    lv_obj_t* labelTituloLog = lv_label_create(containerLogs);
    lv_label_set_text(labelTituloLog, "Logs Recentes de Rede:");
    lv_obj_set_style_text_color(labelTituloLog, lv_palette_main(LV_PALETTE_YELLOW), 0);

    // [CRITICO] Alocando os Labels estaticamente no boot. Nada e recriado em run-time!
    for (int i = 0; i < MAX_LOGS_UI; i++) {
        _labelsLog[i] = lv_label_create(containerLogs);
        lv_label_set_text(_labelsLog[i], "-");
        lv_obj_add_style(_labelsLog[i], &_estiloFontePequena, 0);
    }

    // ==========================================
    // ABAS 2: Regra de Negocio do M4 Inteligente
    // ==========================================
    lv_obj_t* containerM4 = lv_obj_create(_telaMenu);
    lv_obj_set_size(containerM4, 220, 100);
    lv_obj_align(containerM4, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_style(containerM4, &_estiloCard, 0);

    lv_obj_t* labelTituloM4 = lv_label_create(containerM4);
    lv_label_set_text(labelTituloM4, "Atuadores (M4)");
    lv_obj_align(labelTituloM4, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_color(labelTituloM4, lv_color_white(), 0);

    _btnMenuM4 = lv_btn_create(containerM4);
    lv_obj_align(_btnMenuM4, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_style(_btnMenuM4, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(_btnMenuM4, _btnM4ClickCb, LV_EVENT_CLICKED, this);
    lv_obj_t* btnM4Label = lv_label_create(_btnMenuM4);
    lv_label_set_text(btnM4Label, "Listar Biometrias");

    _labelAvisoM4 = lv_label_create(containerM4);
    lv_label_set_text(_labelAvisoM4, "Sem Modulo 4 na Rede");
    lv_obj_add_style(_labelAvisoM4, &_estiloFontePequena, 0);
    lv_obj_align(_labelAvisoM4, LV_ALIGN_BOTTOM_MID, 0, -15);

    // ==========================================
    // Botoes Fixos
    // ==========================================
    lv_obj_t* btnVoltar = lv_btn_create(_telaMenu);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btnVoltar, _btnDashboardClickCb, LV_EVENT_CLICKED, this);
    lv_obj_t* voltarLabel = lv_label_create(btnVoltar);
    lv_label_set_text(voltarLabel, "<- Voltar ao Dash");
}

void scGestorTelasM3::_construirAlerta() {
    lv_obj_add_style(_telaAlerta, &_estiloAlerta, 0);

    lv_obj_t* label = lv_label_create(_telaAlerta);
    lv_label_set_text(label, "EMERGENCIA GERAL!\nVazamento/Falha Critica detectada.\nAbafe Imediatamente!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void scGestorTelasM3::mostrarDashboard() {
    lv_scr_load(_telaDashboard); // Troca atômica de ponteiro de tela (Zero Heap)
}

void scGestorTelasM3::mostrarMenu() {
    atualizarMenuInteligente();
    lv_scr_load(_telaMenu);
}

void scGestorTelasM3::dispararAlertaVermelho(const char* mensagem) {
    lv_scr_load(_telaAlerta); // Sobrepõe tudo e trava a maquina de estado grafica
}

void scGestorTelasM3::atualizarMenuInteligente() {
    if (_radar == nullptr) return;

    if (_radar->existeModuloDoTipo("M4")) {
        // Apresenta a funcionalidade e oculta o aviso
        lv_obj_clear_flag(_btnMenuM4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(_labelAvisoM4, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Recolhe o botao inutil e mostra o label passivo
        lv_obj_add_flag(_btnMenuM4, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(_labelAvisoM4, LV_OBJ_FLAG_HIDDEN);
    }
}

void scGestorTelasM3::adicionarLogRede(const char* mac, const char* descricao) {
    // 1. Shift-Right estatico no Array (Descarta o 9, move o 8 pro 9, etc)
    for (int i = MAX_LOGS_UI - 1; i > 0; i--) {
        strncpy(_historicoLogs[i], _historicoLogs[i-1], TAM_LOG_UI);
    }

    // 2. Imprime na cabeca do buffer fisico
    snprintf(_historicoLogs[0], TAM_LOG_UI, "[%c%c..] %s", mac[0], mac[1], descricao);

    // 3. Empurra os blocos de texto (Strings char*) limpos de volta para os Labels pre-alocados.
    // O LVGL simplesmente repinta os blocos de tela invalidados sem "dar free" em obj_t.
    for (int i = 0; i < MAX_LOGS_UI; i++) {
        if (_labelsLog[i] != nullptr) {
            lv_label_set_text(_labelsLog[i], _historicoLogs[i]);
        }
    }
}

// =======================================================
// CALLBACKS EVENT DRIVEN (Acionados fisicamente pela tela)
// =======================================================

void scGestorTelasM3::_btnMenuClickCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    if (instance) instance->mostrarMenu();
}

void scGestorTelasM3::_btnDashboardClickCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    if (instance) instance->mostrarDashboard();
}

void scGestorTelasM3::_btnM4ClickCb(lv_event_t* e) {
    // Callback disparado ao clicar no botao de listar Biometrias
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    if (instance && instance->_logger) {
        instance->_logger->info("scGestorTelas", "Aba de modulos atuadores bio aberta.");
    }
}
