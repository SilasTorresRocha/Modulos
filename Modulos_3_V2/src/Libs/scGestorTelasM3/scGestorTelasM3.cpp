#include "scGestorTelasM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scRadarEcossistemaM3/scRadarEcossistemaM3.h"
#include "../scGestorDispositivosM3/scGestorDispositivosM3.h"
#include "../../LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "../../LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"
#include "../scRoteadorBridgeM3/scRoteadorBridgeM3.h"
#include <ArduinoJson.h>

extern "C" const lv_font_t lv_font_montserrat_48;

// Estilos Globais Estaticos do Motor C
static lv_style_t _estiloCard;
static lv_style_t _estiloBotaoAzul;
static lv_style_t _estiloBotaoVoltar;
static lv_style_t _estiloAlerta;
static lv_style_t _estiloFontePequena;

scGestorTelasM3::scGestorTelasM3() : 
    _logger(nullptr), _radar(nullptr), _gestorDispositivos(nullptr),
    _telaDashboardCompleto(nullptr), _telaDashboardSimplificado(nullptr),
    _telaMenuPrincipal(nullptr), _telaLogs(nullptr), _telaConfigRede(nullptr),
    _telaConfigM1(nullptr), _telaConfigM2(nullptr), _telaConfigM4(nullptr),
    _telaAlerta(nullptr),
    _cardM1(nullptr), _cardM2(nullptr), _cardM4(nullptr),
    _labelM1Info(nullptr), _labelM2Info(nullptr), _labelM4Info(nullptr),
    _btnMenuM1(nullptr), _btnMenuM2(nullptr), _btnMenuM4(nullptr),
    _labelMenuAvisoEcosistema(nullptr),
    _tecladoVirtual(nullptr), _ddlWifi(nullptr), _txtSenhaWifi(nullptr),
    _txtMqttUser(nullptr), _txtMqttPass(nullptr), _lblStatusConfig(nullptr),
    _btnSalvarRede(nullptr), _btnVoltarRede(nullptr), _labelHora(nullptr), _ultimaHoraCache(""),
    _labelWifiRSSI(nullptr), _eeprom(nullptr), _mqtt(nullptr), _rtc(nullptr), _rede(nullptr), _roteador(nullptr) 
{
    for (int i = 0; i < MAX_LOGS_UI; i++) {
        _labelsLog[i] = nullptr;
        memset(_historicoLogs[i], 0, TAM_LOG_UI);
    }
}

void scGestorTelasM3::inicializar(scLogger* logger, scRadarEcossistemaM3* radar, scGestorDispositivosM3* disp, scArmazenamentoLocal* eeprom, scMQTTLib* mqtt, scRTCFisicoM3* rtc, scGestorRede* rede, scRoteadorBridgeM3* roteador) {
    _logger = logger;
    _radar = radar;
    _gestorDispositivos = disp;
    _eeprom = eeprom;
    _mqtt = mqtt;
    _rtc = rtc;
    _rede = rede;
    _roteador = roteador;

    _construirEstilosPadrao();

    // Instancia as telas-base
    _telaDashboardCompleto = lv_obj_create(NULL);
    _telaDashboardSimplificado = lv_obj_create(NULL);
    _telaMenuPrincipal = lv_obj_create(NULL);
    _telaLogs = lv_obj_create(NULL);
    _telaConfigRede = lv_obj_create(NULL);
    _telaConfigM1 = lv_obj_create(NULL);
    _telaConfigM2 = lv_obj_create(NULL);
    _telaConfigM4 = lv_obj_create(NULL);
    _telaAlerta = lv_obj_create(NULL);

    _construirDashboardCompleto();
    _construirDashboardSimplificado();
    _construirMenuPrincipal();
    _construirTelaLogs();
    _construirTelaConfigRede();
    _construirTelaConfigM1();
    _construirTelaConfigM2();
    _construirTelaConfigM4();
    _construirAlerta();

    mostrarDashboardCompleto();

    if (_logger != nullptr) _logger->info("scGestorTelas", "UI LVGL pre-alocada e estavel. Dashboards nativos criados.");
}

void scGestorTelasM3::_construirEstilosPadrao() {
    lv_style_init(&_estiloCard);
    lv_style_set_bg_color(&_estiloCard, lv_color_hex(0x202020));
    lv_style_set_radius(&_estiloCard, 10);
    lv_style_set_border_width(&_estiloCard, 2);
    lv_style_set_border_color(&_estiloCard, lv_color_hex(0x555555));

    lv_style_init(&_estiloBotaoAzul);
    lv_style_set_bg_color(&_estiloBotaoAzul, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_radius(&_estiloBotaoAzul, 5);
    lv_style_set_text_color(&_estiloBotaoAzul, lv_color_white());

    lv_style_init(&_estiloBotaoVoltar);
    lv_style_set_bg_color(&_estiloBotaoVoltar, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_radius(&_estiloBotaoVoltar, 5);
    lv_style_set_text_color(&_estiloBotaoVoltar, lv_color_white());

    lv_style_init(&_estiloAlerta);
    lv_style_set_bg_color(&_estiloAlerta, lv_palette_main(LV_PALETTE_RED));
    lv_style_set_text_color(&_estiloAlerta, lv_color_white());
    
    lv_style_init(&_estiloFontePequena);
    lv_style_set_text_color(&_estiloFontePequena, lv_color_hex(0xAAAAAA));
}

void scGestorTelasM3::_construirDashboardCompleto() {
    lv_obj_set_style_bg_color(_telaDashboardCompleto, lv_color_black(), 0);

    lv_obj_t* labelTitulo = lv_label_create(_telaDashboardCompleto);
    lv_label_set_text(labelTitulo, "Visao Geral - Malha IoT");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    // Indicador de Sinal Wi-Fi
    _labelWifiRSSI = lv_label_create(_telaDashboardCompleto);
    lv_label_set_text(_labelWifiRSSI, "WiFi: -- dBm");
    lv_obj_align(_labelWifiRSSI, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_text_color(_labelWifiRSSI, lv_palette_main(LV_PALETTE_CYAN), 0);

    // Layout de Cards (M1, M2, M4)
    lv_obj_t* grid = lv_obj_create(_telaDashboardCompleto);
    lv_obj_set_size(grid, 460, 220);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Card M1
    _cardM1 = lv_obj_create(grid);
    lv_obj_set_size(_cardM1, 140, 100);
    lv_obj_add_style(_cardM1, &_estiloCard, 0);
    lv_obj_t* lM1 = lv_label_create(_cardM1);
    lv_label_set_text(lM1, "M1 (Sensores)");
    lv_obj_align(lM1, LV_ALIGN_TOP_MID, 0, 5);
    _labelM1Info = lv_label_create(_cardM1);
    lv_label_set_text(_labelM1Info, "Temp: -- C\nGas: --");
    lv_obj_align(_labelM1Info, LV_ALIGN_CENTER, 0, 10);

    // Card M2
    _cardM2 = lv_obj_create(grid);
    lv_obj_set_size(_cardM2, 140, 100);
    lv_obj_add_style(_cardM2, &_estiloCard, 0);
    lv_obj_t* lM2 = lv_label_create(_cardM2);
    lv_label_set_text(lM2, "M2 (Relés)");
    lv_obj_align(lM2, LV_ALIGN_TOP_MID, 0, 5);
    _labelM2Info = lv_label_create(_cardM2);
    lv_label_set_text(_labelM2Info, "Consumo: -- W\nR1:OFF R2:OFF");
    lv_obj_align(_labelM2Info, LV_ALIGN_CENTER, 0, 10);

    // Card M4
    _cardM4 = lv_obj_create(grid);
    lv_obj_set_size(_cardM4, 140, 100);
    lv_obj_add_style(_cardM4, &_estiloCard, 0);
    lv_obj_t* lM4 = lv_label_create(_cardM4);
    lv_label_set_text(lM4, "M4 (Cofre)");
    lv_obj_align(lM4, LV_ALIGN_TOP_MID, 0, 5);
    _labelM4Info = lv_label_create(_cardM4);
    lv_label_set_text(_labelM4Info, "Porta: Fechada\nBateria: --%");
    lv_obj_align(_labelM4Info, LV_ALIGN_CENTER, 0, 10);

    // Botão de Configuração
    lv_obj_t* btn = lv_btn_create(_telaDashboardCompleto);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(btn, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btn, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_obj_t* btnLabel = lv_label_create(btn);
    lv_label_set_text(btnLabel, "Menu de Sistema");
}

void scGestorTelasM3::_construirDashboardSimplificado() {
    lv_obj_set_style_bg_color(_telaDashboardSimplificado, lv_color_black(), 0);

    _labelHora = lv_label_create(_telaDashboardSimplificado);
    lv_label_set_text(_labelHora, "12:00");
    lv_obj_set_style_text_font(_labelHora, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(_labelHora, lv_color_white(), 0);
    lv_obj_align(_labelHora, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t* labelInfo = lv_label_create(_telaDashboardSimplificado);
    lv_label_set_text(labelInfo, "Casa Segura - Toque para acordar");
    lv_obj_set_style_text_color(labelInfo, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(labelInfo, LV_ALIGN_CENTER, 0, 40);

    // Botão Overlay Invisível para evitar o engasgo/crash do Motor LVGL (overflow do render raiz)
    lv_obj_t* overlayBtn = lv_btn_create(_telaDashboardSimplificado);
    lv_obj_set_size(overlayBtn, 480, 320); // Tamanho da tela
    lv_obj_set_style_bg_opa(overlayBtn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(overlayBtn, 0, 0);
    lv_obj_set_style_shadow_width(overlayBtn, 0, 0);
    lv_obj_add_event_cb(overlayBtn, _btnIrParaDashboardCompletoCb, LV_EVENT_CLICKED, this);
}

void scGestorTelasM3::_construirMenuPrincipal() {
    lv_obj_set_style_bg_color(_telaMenuPrincipal, lv_color_black(), 0);

    lv_obj_t* labelTitulo = lv_label_create(_telaMenuPrincipal);
    lv_label_set_text(labelTitulo, "Roteamento e Configuracoes");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    lv_obj_t* grid = lv_obj_create(_telaMenuPrincipal);
    lv_obj_set_size(grid, 460, 200);
    lv_obj_align(grid, LV_ALIGN_CENTER, 0, 10);
    lv_obj_set_style_bg_opa(grid, 0, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_layout(grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    // Hub Config
    lv_obj_t* btnHub = lv_btn_create(grid);
    lv_obj_set_size(btnHub, 140, 50);
    lv_obj_add_style(btnHub, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btnHub, _btnIrParaConfigRedeCb, LV_EVENT_CLICKED, this);
    lv_obj_t* lblHub = lv_label_create(btnHub);
    lv_label_set_text(lblHub, "Config. Wi-Fi");
    lv_obj_center(lblHub);

    // Logs
    lv_obj_t* btnLogs = lv_btn_create(grid);
    lv_obj_set_size(btnLogs, 140, 50);
    lv_obj_add_style(btnLogs, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btnLogs, _btnIrParaLogsCb, LV_EVENT_CLICKED, this);
    lv_obj_t* lblLogs = lv_label_create(btnLogs);
    lv_label_set_text(lblLogs, "Logs de Rede");
    lv_obj_center(lblLogs);

    // Dinâmicos (Iniciam ocultos)
    _btnMenuM1 = lv_btn_create(grid);
    lv_obj_set_size(_btnMenuM1, 140, 50);
    lv_obj_add_style(_btnMenuM1, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(_btnMenuM1, _btnIrParaConfigM1Cb, LV_EVENT_CLICKED, this);
    lv_obj_t* lblM1 = lv_label_create(_btnMenuM1);
    lv_label_set_text(lblM1, "Config. M1");
    lv_obj_center(lblM1);
    lv_obj_add_flag(_btnMenuM1, LV_OBJ_FLAG_HIDDEN);

    _btnMenuM2 = lv_btn_create(grid);
    lv_obj_set_size(_btnMenuM2, 140, 50);
    lv_obj_add_style(_btnMenuM2, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(_btnMenuM2, _btnIrParaConfigM2Cb, LV_EVENT_CLICKED, this);
    lv_obj_t* lblM2 = lv_label_create(_btnMenuM2);
    lv_label_set_text(lblM2, "Config. M2");
    lv_obj_center(lblM2);
    lv_obj_add_flag(_btnMenuM2, LV_OBJ_FLAG_HIDDEN);

    _btnMenuM4 = lv_btn_create(grid);
    lv_obj_set_size(_btnMenuM4, 140, 50);
    lv_obj_add_style(_btnMenuM4, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(_btnMenuM4, _btnIrParaConfigM4Cb, LV_EVENT_CLICKED, this);
    lv_obj_t* lblM4 = lv_label_create(_btnMenuM4);
    lv_label_set_text(lblM4, "Config. M4");
    lv_obj_center(lblM4);
    lv_obj_add_flag(_btnMenuM4, LV_OBJ_FLAG_HIDDEN);

    // Aviso do Ecossistema
    _labelMenuAvisoEcosistema = lv_label_create(_telaMenuPrincipal);
    lv_label_set_text(_labelMenuAvisoEcosistema, "Verificando Malha...");
    lv_obj_add_style(_labelMenuAvisoEcosistema, &_estiloFontePequena, 0);
    lv_obj_align(_labelMenuAvisoEcosistema, LV_ALIGN_BOTTOM_RIGHT, -10, -20);

    lv_obj_t* btnVoltar = lv_btn_create(_telaMenuPrincipal);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(btnVoltar, _btnIrParaDashboardCompletoCb, LV_EVENT_CLICKED, this);
    lv_obj_t* voltarLabel = lv_label_create(btnVoltar);
    lv_label_set_text(voltarLabel, "<- Dashboard");
}

void scGestorTelasM3::_construirTelaLogs() {
    lv_obj_set_style_bg_color(_telaLogs, lv_color_black(), 0);

    lv_obj_t* labelTituloLog = lv_label_create(_telaLogs);
    lv_label_set_text(labelTituloLog, "Logs Recentes de Rede");
    lv_obj_align(labelTituloLog, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTituloLog, lv_palette_main(LV_PALETTE_YELLOW), 0);

    lv_obj_t* containerLogs = lv_obj_create(_telaLogs);
    lv_obj_set_size(containerLogs, 460, 220);
    lv_obj_align(containerLogs, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(containerLogs, &_estiloCard, 0);
    lv_obj_set_layout(containerLogs, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(containerLogs, LV_FLEX_FLOW_COLUMN);

    for (int i = 0; i < MAX_LOGS_UI; i++) {
        _labelsLog[i] = lv_label_create(containerLogs);
        lv_label_set_text(_labelsLog[i], "-");
        lv_obj_add_style(_labelsLog[i], &_estiloFontePequena, 0);
    }

    lv_obj_t* btnVoltar = lv_btn_create(_telaLogs);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(btnVoltar, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_obj_t* voltarLabel = lv_label_create(btnVoltar);
    lv_label_set_text(voltarLabel, "<- Voltar");
}

void scGestorTelasM3::_construirTelaConfigRede() {
    lv_obj_set_style_bg_color(_telaConfigRede, lv_color_black(), 0);

    lv_obj_t* labelTitulo = lv_label_create(_telaConfigRede);
    lv_label_set_text(labelTitulo, "Configuracoes da Malha (Hub)");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    // Botao Escanear
    lv_obj_t* btnEscanear = lv_btn_create(_telaConfigRede);
    lv_obj_align(btnEscanear, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_add_style(btnEscanear, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(btnEscanear, _btnEscanearCb, LV_EVENT_CLICKED, this);
    lv_label_set_text(lv_label_create(btnEscanear), "Escanear Redes");

    // Dropdown de Redes
    _ddlWifi = lv_dropdown_create(_telaConfigRede);
    lv_dropdown_set_options(_ddlWifi, "Toque em Escanear...");
    lv_obj_set_width(_ddlWifi, 200);
    lv_obj_align(_ddlWifi, LV_ALIGN_TOP_LEFT, 10, 80);

    // Senha Wi-Fi
    _txtSenhaWifi = lv_textarea_create(_telaConfigRede);
    lv_obj_set_size(_txtSenhaWifi, 200, 40);
    lv_obj_align(_txtSenhaWifi, LV_ALIGN_TOP_LEFT, 10, 130);
    lv_textarea_set_placeholder_text(_txtSenhaWifi, "Senha do Wi-Fi");
    lv_textarea_set_password_mode(_txtSenhaWifi, true);
    lv_obj_add_event_cb(_txtSenhaWifi, _taFocusCb, LV_EVENT_FOCUSED, this);

    // MQTT User
    _txtMqttUser = lv_textarea_create(_telaConfigRede);
    lv_obj_set_size(_txtMqttUser, 200, 40);
    lv_obj_align(_txtMqttUser, LV_ALIGN_TOP_RIGHT, -10, 80);
    lv_textarea_set_placeholder_text(_txtMqttUser, "Usuario MQTT");
    lv_obj_add_event_cb(_txtMqttUser, _taFocusCb, LV_EVENT_FOCUSED, this);

    // MQTT Pass
    _txtMqttPass = lv_textarea_create(_telaConfigRede);
    lv_obj_set_size(_txtMqttPass, 200, 40);
    lv_obj_align(_txtMqttPass, LV_ALIGN_TOP_RIGHT, -10, 130);
    lv_textarea_set_placeholder_text(_txtMqttPass, "Senha MQTT");
    lv_textarea_set_password_mode(_txtMqttPass, true);
    lv_obj_add_event_cb(_txtMqttPass, _taFocusCb, LV_EVENT_FOCUSED, this);

    // Label Status
    _lblStatusConfig = lv_label_create(_telaConfigRede);
    lv_label_set_text(_lblStatusConfig, "");
    lv_obj_align(_lblStatusConfig, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_set_style_text_color(_lblStatusConfig, lv_palette_main(LV_PALETTE_YELLOW), 0);

    // Teclado
    _tecladoVirtual = lv_keyboard_create(_telaConfigRede);
    lv_obj_add_flag(_tecladoVirtual, LV_OBJ_FLAG_HIDDEN);
    // Fechar teclado ao clicar no Check
    lv_obj_add_event_cb(_tecladoVirtual, [](lv_event_t* e){
        scGestorTelasM3* inst = (scGestorTelasM3*)lv_event_get_user_data(e);
        lv_obj_t* kb = lv_event_get_target(e);
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        if(inst->_btnSalvarRede) lv_obj_clear_flag(inst->_btnSalvarRede, LV_OBJ_FLAG_HIDDEN);
        if(inst->_btnVoltarRede) lv_obj_clear_flag(inst->_btnVoltarRede, LV_OBJ_FLAG_HIDDEN);
    }, LV_EVENT_READY, this);

    // Botao Salvar e Propagar
    _btnSalvarRede = lv_btn_create(_telaConfigRede);
    lv_obj_align(_btnSalvarRede, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_add_style(_btnSalvarRede, &_estiloBotaoAzul, 0);
    lv_obj_add_event_cb(_btnSalvarRede, _btnSalvarConfigCb, LV_EVENT_CLICKED, this);
    lv_label_set_text(lv_label_create(_btnSalvarRede), "Testar & Propagar Malha");

    _btnVoltarRede = lv_btn_create(_telaConfigRede);
    lv_obj_align(_btnVoltarRede, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(_btnVoltarRede, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(_btnVoltarRede, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_obj_t* voltarLabel = lv_label_create(_btnVoltarRede);
    lv_label_set_text(voltarLabel, "<- Voltar");
}

void scGestorTelasM3::_construirTelaConfigM1() {
    lv_obj_set_style_bg_color(_telaConfigM1, lv_color_black(), 0);
    lv_obj_t* labelTitulo = lv_label_create(_telaConfigM1);
    lv_label_set_text(labelTitulo, "Configuracoes M1 (Gas)");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    lv_obj_t* btnVoltar = lv_btn_create(_telaConfigM1);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(btnVoltar, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_label_set_text(lv_label_create(btnVoltar), "<- Voltar");
}

void scGestorTelasM3::_construirTelaConfigM2() {
    lv_obj_set_style_bg_color(_telaConfigM2, lv_color_black(), 0);
    lv_obj_t* labelTitulo = lv_label_create(_telaConfigM2);
    lv_label_set_text(labelTitulo, "Configuracoes M2 (Reles)");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    lv_obj_t* btnVoltar = lv_btn_create(_telaConfigM2);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(btnVoltar, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_label_set_text(lv_label_create(btnVoltar), "<- Voltar");
}

void scGestorTelasM3::_construirTelaConfigM4() {
    lv_obj_set_style_bg_color(_telaConfigM4, lv_color_black(), 0);
    lv_obj_t* labelTitulo = lv_label_create(_telaConfigM4);
    lv_label_set_text(labelTitulo, "Configuracoes M4 (Biometria)");
    lv_obj_align(labelTitulo, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(labelTitulo, lv_color_white(), 0);

    lv_obj_t* btnVoltar = lv_btn_create(_telaConfigM4);
    lv_obj_align(btnVoltar, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_add_style(btnVoltar, &_estiloBotaoVoltar, 0);
    lv_obj_add_event_cb(btnVoltar, _btnIrParaMenuPrincipalCb, LV_EVENT_CLICKED, this);
    lv_label_set_text(lv_label_create(btnVoltar), "<- Voltar");
}

void scGestorTelasM3::_construirAlerta() {
    lv_obj_add_style(_telaAlerta, &_estiloAlerta, 0);
    lv_obj_t* labelAlerta = lv_label_create(_telaAlerta);
    lv_label_set_text(labelAlerta, "ALERTA CRITICO");
    lv_obj_align(labelAlerta, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(_telaAlerta, _btnIrParaDashboardCompletoCb, LV_EVENT_CLICKED, this);
}

// ==========================================
// CALLBACKS E EVENTOS
// ==========================================

void scGestorTelasM3::_btnIrParaMenuPrincipalCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    instance->mostrarMenuPrincipal();
}

void scGestorTelasM3::_btnIrParaDashboardCompletoCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    instance->mostrarDashboardCompleto();
}

void scGestorTelasM3::_btnIrParaLogsCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_scr_load(instance->_telaLogs);
}

void scGestorTelasM3::_btnIrParaConfigRedeCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_scr_load(instance->_telaConfigRede);
}

void scGestorTelasM3::_btnIrParaConfigM1Cb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_scr_load(instance->_telaConfigM1);
}

void scGestorTelasM3::_btnIrParaConfigM2Cb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_scr_load(instance->_telaConfigM2);
}

void scGestorTelasM3::_btnIrParaConfigM4Cb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_scr_load(instance->_telaConfigM4);
}

void scGestorTelasM3::_taFocusCb(lv_event_t* e) {
    scGestorTelasM3* instance = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_obj_t* ta = lv_event_get_target(e);
    lv_keyboard_set_textarea(instance->_tecladoVirtual, ta);
    lv_obj_clear_flag(instance->_tecladoVirtual, LV_OBJ_FLAG_HIDDEN);
    
    // Oculta botoes de baixo para nao sobrepor o teclado
    if(instance->_btnSalvarRede) lv_obj_add_flag(instance->_btnSalvarRede, LV_OBJ_FLAG_HIDDEN);
    if(instance->_btnVoltarRede) lv_obj_add_flag(instance->_btnVoltarRede, LV_OBJ_FLAG_HIDDEN);
}

void scGestorTelasM3::_btnEscanearCb(lv_event_t* e) {
    scGestorTelasM3* inst = (scGestorTelasM3*)lv_event_get_user_data(e);
    lv_dropdown_set_options(inst->_ddlWifi, "Escaneando...");
    
    // Dispara scan assincrono
    WiFi.scanNetworks(true);
    
    // Loop bloqueante rapido apenas para aguardar (no S2 leva ~1.5s). Nao ideal para Super Loop, mas aceitavel em Setup
    int16_t n = WiFi.scanComplete();
    unsigned long inicio = millis();
    while(n == WIFI_SCAN_RUNNING && millis() - inicio < 4000) {
        delay(50);
        n = WiFi.scanComplete();
    }
    
    if (n > 0) {
        String options = "";
        for (int i = 0; i < n; ++i) {
            options += WiFi.SSID(i);
            if (i < n - 1) options += "\n";
        }
        lv_dropdown_set_options(inst->_ddlWifi, options.c_str());
    } else {
        lv_dropdown_set_options(inst->_ddlWifi, "Nenhuma rede achada");
    }
    WiFi.scanDelete();
}

void scGestorTelasM3::_btnSalvarConfigCb(lv_event_t* e) {
    scGestorTelasM3* inst = (scGestorTelasM3*)lv_event_get_user_data(e);
    
    if (!inst->_eeprom || !inst->_mqtt || !inst->_roteador) return;

    lv_label_set_text(inst->_lblStatusConfig, "Testando Wi-Fi...");
    lv_timer_handler(); // Forca atualizar a UI
    delay(10); // Respiro

    char ssid[32];
    lv_dropdown_get_selected_str(inst->_ddlWifi, ssid, sizeof(ssid));
    String pass = lv_textarea_get_text(inst->_txtSenhaWifi);
    String mqttU = lv_textarea_get_text(inst->_txtMqttUser);
    String mqttP = lv_textarea_get_text(inst->_txtMqttPass);

    // 1. Testa a rede (Desconecta da atual e tenta a nova bloqueante ate 10s)
    WiFi.disconnect();
    WiFi.begin(ssid, pass.c_str());
    int timeout = 20;
    while(WiFi.status() != WL_CONNECTED && timeout > 0) {
        lv_timer_handler(); // Mantem a UI viva durante o teste
        delay(500);
        timeout--;
    }

    if (WiFi.status() != WL_CONNECTED) {
        lv_label_set_text(inst->_lblStatusConfig, "Erro! Senha invalida. Revertendo...");
        lv_obj_set_style_text_color(inst->_lblStatusConfig, lv_palette_main(LV_PALETTE_RED), 0);
        lv_timer_handler(); // Forca atualizar a UI para o usuario ver o erro
        
        // O ESP.restart revertara para a EEPROM que não foi modificada
        delay(3000); // 3 segundos para o usuario ler
        ESP.restart();
        return;
    }

    // 2. Passou no teste! Salva na Flash
    inst->_eeprom->salvarChaveValor("WIFI_SSID", ssid);
    inst->_eeprom->salvarChaveValor("WIFI_PASS", pass);
    inst->_eeprom->salvarChaveValor("MQTT_USER", mqttU);
    inst->_eeprom->salvarChaveValor("MQTT_PASS", mqttP);

    // 3. Propagação (Broadcast)
    lv_label_set_text(inst->_lblStatusConfig, "Salvo! Propagando para a malha...");
    lv_obj_set_style_text_color(inst->_lblStatusConfig, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_timer_handler();

    StaticJsonDocument<256> docWifi;
    docWifi["mac_origem"] = "HUB";
    docWifi["mac_destino"] = "ALL";
    docWifi["cmd"] = "update_wifi";
    JsonObject argsWifi = docWifi.createNestedObject("args");
    argsWifi["ssid"] = ssid;
    argsWifi["pass"] = pass;
    char bufWifi[256];
    serializeJson(docWifi, bufWifi);
    inst->_roteador->enviarBroadcast(bufWifi); // Pelo ESP-NOW para filhos offline
    inst->_mqtt->enviarJSON(bufWifi);          // Pelo MQTT (Telemetria) para o Backend propagar
    
    StaticJsonDocument<256> docMqtt;
    docMqtt["mac_origem"] = "HUB";
    docMqtt["mac_destino"] = "ALL";
    docMqtt["cmd"] = "update_libmqtt";
    JsonObject argsMqtt = docMqtt.createNestedObject("args");
    argsMqtt["usuario"] = mqttU;
    argsMqtt["senha"] = mqttP;
    char bufMqtt[256];
    serializeJson(docMqtt, bufMqtt);
    inst->_roteador->enviarBroadcast(bufMqtt); // Pelo ESP-NOW para filhos offline
    inst->_mqtt->enviarJSON(bufMqtt);          // Pelo MQTT (Telemetria) para o Backend propagar
    
    delay(3000); // 3 segundos para o usuario ler o "Salvo!"
    ESP.restart(); // Aplica mudancas
}

// ==========================================
// LOGICA DE CONTROLE
// ==========================================

void scGestorTelasM3::mostrarDashboardCompleto() {
    lv_scr_load(_telaDashboardCompleto);
}

void scGestorTelasM3::mostrarDashboardSimplificado() {
    lv_scr_load(_telaDashboardSimplificado);
}

void scGestorTelasM3::mostrarMenuPrincipal() {
    atualizarMenuInteligente();
    lv_scr_load(_telaMenuPrincipal);
}

void scGestorTelasM3::dispararAlertaVermelho(const char* mensagem) {
    lv_obj_t* label = lv_obj_get_child(_telaAlerta, 0);
    if (label) lv_label_set_text(label, mensagem);
    lv_scr_load(_telaAlerta);
}

void scGestorTelasM3::adicionarLogRede(const char* mac, const char* descricao) {
    // Desloca histórico (Ring Buffer em RAM)
    for (int i = MAX_LOGS_UI - 1; i > 0; i--) {
        strncpy(_historicoLogs[i], _historicoLogs[i - 1], TAM_LOG_UI);
        if (_labelsLog[i]) lv_label_set_text(_labelsLog[i], _historicoLogs[i]);
    }
    snprintf(_historicoLogs[0], TAM_LOG_UI, "[%s] %s", mac, descricao);
    if (_labelsLog[0]) lv_label_set_text(_labelsLog[0], _historicoLogs[0]);
}

void scGestorTelasM3::_aplicarCorCard(lv_obj_t* card, int statusRadar) {
    if (!card) return;
    
    // StatusRadar: 0 = VERDE, 1 = AMARELO, 2 = VERMELHO
    lv_color_t corBorda;
    if (statusRadar == VERDE_ONLINE) corBorda = lv_palette_main(LV_PALETTE_GREEN);
    else if (statusRadar == AMARELO_FALLBACK) corBorda = lv_palette_main(LV_PALETTE_YELLOW);
    else corBorda = lv_palette_main(LV_PALETTE_RED);

    lv_obj_set_style_border_color(card, corBorda, 0);
}

void scGestorTelasM3::atualizarMenuInteligente() {
    if (!_radar) return;

    bool m1Ok = _radar->existeModuloDoTipo("M1");
    bool m2Ok = _radar->existeModuloDoTipo("M2");
    bool m4Ok = _radar->existeModuloDoTipo("M4");

    // Lógica dos Botões de Configuração
    if (m1Ok) lv_obj_clear_flag(_btnMenuM1, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(_btnMenuM1, LV_OBJ_FLAG_HIDDEN);

    if (m2Ok) lv_obj_clear_flag(_btnMenuM2, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(_btnMenuM2, LV_OBJ_FLAG_HIDDEN);

    if (m4Ok) lv_obj_clear_flag(_btnMenuM4, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(_btnMenuM4, LV_OBJ_FLAG_HIDDEN);

    // Lógica do Aviso Geral
    if (!m1Ok && !m2Ok && !m4Ok) {
        lv_label_set_text(_labelMenuAvisoEcosistema, "Aviso: Sem Nenhuma Placa Filha (Malha Vazia)");
        lv_obj_set_style_text_color(_labelMenuAvisoEcosistema, lv_palette_main(LV_PALETTE_YELLOW), 0);
    } else {
        lv_label_set_text(_labelMenuAvisoEcosistema, "Ecossistema Saudavel (P2P/MQTT Integrados)");
        lv_obj_set_style_text_color(_labelMenuAvisoEcosistema, lv_palette_main(LV_PALETTE_GREEN), 0);
    }

    // Lógica de Cores dos Dashboards
    _aplicarCorCard(_cardM1, _radar->obterEstadoPorTipo("M1"));
    _aplicarCorCard(_cardM2, _radar->obterEstadoPorTipo("M2"));
    _aplicarCorCard(_cardM4, _radar->obterEstadoPorTipo("M4"));
}

void scGestorTelasM3::processar() {
    // 1. Atualizar Hora no Dashboard Simplificado se estiver na tela
    if (_rtc && _labelHora) {
        String novaHora = _rtc->obterHoraFormatada();
        if (novaHora != _ultimaHoraCache) {
            _ultimaHoraCache = novaHora;
            lv_label_set_text(_labelHora, novaHora.c_str());
        }
    }

    // Atualizar Sinal Wi-Fi no Dashboard Completo (a cada 2 segundos via logica interna ou so quando on-screen)
    if (_rede && _labelWifiRSSI) {
        static uint32_t ultimoRssiUpdate = 0;
        if (millis() - ultimoRssiUpdate > 2000) {
            ultimoRssiUpdate = millis();
            int rssi = _rede->obterRssi();
            char buf[32];
            if (rssi == -100) {
                snprintf(buf, sizeof(buf), "WiFi: Off");
            } else {
                snprintf(buf, sizeof(buf), "WiFi: %d dBm", rssi);
            }
            lv_label_set_text(_labelWifiRSSI, buf);
        }
    }

    // 2. Fallback de inatividade
    if (lv_disp_get_inactive_time(NULL) > 60000) {
        lv_obj_t* telaAtual = lv_scr_act();
        if (telaAtual != _telaDashboardCompleto && telaAtual != _telaDashboardSimplificado) {
            mostrarDashboardSimplificado();
        }
    }
}
