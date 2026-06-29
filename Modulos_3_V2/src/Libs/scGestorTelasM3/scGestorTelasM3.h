#ifndef SC_GESTOR_TELAS_M3_H
#define SC_GESTOR_TELAS_M3_H

#include <Arduino.h>
#include <lvgl.h>

class scLogger;
class scRadarEcossistemaM3;
class scGestorDispositivosM3;
class scArmazenamentoLocal;
class scMQTTLib;
class scRTCFisicoM3;
class scGestorRede;
class scRoteadorBridgeM3;

#define MAX_LOGS_UI 10
#define TAM_LOG_UI 48

class scGestorTelasM3 {
private:
    scLogger* _logger;
    scRadarEcossistemaM3* _radar;
    scGestorDispositivosM3* _gestorDispositivos;
    scArmazenamentoLocal* _eeprom;
    scMQTTLib* _mqtt;
    scRTCFisicoM3* _rtc;
    scGestorRede* _rede;
    scRoteadorBridgeM3* _roteador;

    // Telas Base 
    lv_obj_t* _telaDashboardCompleto;
    lv_obj_t* _telaDashboardSimplificado;
    lv_obj_t* _telaMenuPrincipal;
    lv_obj_t* _telaLogs;
    lv_obj_t* _telaConfigRede;
    lv_obj_t* _telaConfigM1;
    lv_obj_t* _telaConfigM2;
    lv_obj_t* _telaConfigM4;
    lv_obj_t* _telaAlerta;

    // Elementos do Dashboard Completo
    lv_obj_t* _cardM1;
    lv_obj_t* _cardM2;
    lv_obj_t* _cardM4;
    lv_obj_t* _labelM1Info;
    lv_obj_t* _labelM2Info;
    lv_obj_t* _labelM4Info;
    lv_obj_t* _labelWifiRSSI;

    // Elementos do Menu Principal (Roteamento Dinâmico)
    lv_obj_t* _btnMenuM1;
    lv_obj_t* _btnMenuM2;
    lv_obj_t* _btnMenuM4;
    lv_obj_t* _labelMenuAvisoEcosistema;

    // Elementos da Configuração de Rede
    lv_obj_t* _tecladoVirtual;
    lv_obj_t* _ddlWifi;
    lv_obj_t* _txtSenhaWifi;
    lv_obj_t* _txtMqttUser;
    lv_obj_t* _txtMqttPass;
    lv_obj_t* _lblStatusConfig;
    lv_obj_t* _btnSalvarRede;
    lv_obj_t* _btnVoltarRede;
    
    // Elementos Dinâmicos Simplificado
    lv_obj_t* _labelHora;
    String _ultimaHoraCache;

    // Elementos da UI pre-alocados para atualizacao do Ring Buffer Visual
    lv_obj_t* _labelsLog[MAX_LOGS_UI];
    char _historicoLogs[MAX_LOGS_UI][TAM_LOG_UI];

    // Metodos internos de contrucao (Invocados apenas no Boot)
    void _construirEstilosPadrao();
    void _construirDashboardCompleto();
    void _construirDashboardSimplificado();
    void _construirMenuPrincipal();
    void _construirTelaLogs();
    void _construirTelaConfigRede();
    void _construirTelaConfigM1();
    void _construirTelaConfigM2();
    void _construirTelaConfigM4();
    void _construirAlerta();

    // Utilitário interno para mudança de cor do card
    void _aplicarCorCard(lv_obj_t* card, int statusRadar);

    // Callbacks C-Like do LVGL (Necessitam ser estaticos com userdata injection)
    static void _btnIrParaMenuPrincipalCb(lv_event_t* e);
    static void _btnIrParaDashboardCompletoCb(lv_event_t* e);
    static void _btnIrParaLogsCb(lv_event_t* e);
    static void _btnIrParaConfigRedeCb(lv_event_t* e);
    static void _btnIrParaConfigM1Cb(lv_event_t* e);
    static void _btnIrParaConfigM2Cb(lv_event_t* e);
    static void _btnIrParaConfigM4Cb(lv_event_t* e);
    static void _taFocusCb(lv_event_t* e);
    static void _btnEscanearCb(lv_event_t* e);
    static void _btnSalvarConfigCb(lv_event_t* e);

public:
    scGestorTelasM3();

    void inicializar(scLogger* logger, scRadarEcossistemaM3* radar, scGestorDispositivosM3* disp, scArmazenamentoLocal* eeprom, scMQTTLib* mqtt, scRTCFisicoM3* rtc, scGestorRede* rede, scRoteadorBridgeM3* roteador);
    
    // Loop de inatividade
    void processar();

    // Controle Externo (Invocados por hardware triggers, botoes ou MQTT)
    void mostrarDashboardCompleto();
    void mostrarDashboardSimplificado();
    void mostrarMenuPrincipal();
    void dispararAlertaVermelho(const char* mensagem);

    // API de Ingestao de Logs de Rede para preencher o Ring Buffer da UI
    void adicionarLogRede(const char* mac, const char* descricao);
    
    // Atualiza dinamicamente as abas inteligentes de acordo com a demografia da malha
    void atualizarMenuInteligente();
};

#endif
