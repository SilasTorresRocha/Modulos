#ifndef SC_GESTOR_TELAS_M3_H
#define SC_GESTOR_TELAS_M3_H

#include <Arduino.h>
#include <lvgl.h>

class scLogger;
class scRadarEcossistemaM3;
class scGestorDispositivosM3;

#define MAX_LOGS_UI 10
#define TAM_LOG_UI 48

class scGestorTelasM3 {
private:
    scLogger* _logger;
    scRadarEcossistemaM3* _radar;
    scGestorDispositivosM3* _gestorDispositivos;

    // Telas Base (Pre-alocadas para evitar fragmentacao dinamica de memoria)
    lv_obj_t* _telaDashboard;
    lv_obj_t* _telaMenu;
    lv_obj_t* _telaAlerta;

    // Elementos da UI pre-alocados para atualizacao do Ring Buffer Visual
    lv_obj_t* _labelsLog[MAX_LOGS_UI];
    char _historicoLogs[MAX_LOGS_UI][TAM_LOG_UI];
    
    // Componentes de Condicionais Inteligentes
    lv_obj_t* _labelAvisoM4;
    lv_obj_t* _btnMenuM4;

    // Metodos internos de contrucao (Invocados apenas no Boot)
    void _construirEstilosPadrao();
    void _construirDashboard();
    void _construirMenu();
    void _construirAlerta();

    // Callbacks C-Like do LVGL (Necessitam ser estaticos com userdata injection)
    static void _btnMenuClickCb(lv_event_t* e);
    static void _btnDashboardClickCb(lv_event_t* e);
    static void _btnM4ClickCb(lv_event_t* e);

public:
    scGestorTelasM3();

    void inicializar(scLogger* logger, scRadarEcossistemaM3* radar, scGestorDispositivosM3* disp);

    // Controle Externo (Invocados por hardware triggers, botoes ou MQTT)
    void mostrarDashboard();
    void mostrarMenu();
    void dispararAlertaVermelho(const char* mensagem);

    // API de Ingestao de Logs de Rede para preencher o Ring Buffer da UI
    void adicionarLogRede(const char* mac, const char* descricao);
    
    // Atualiza dinamicamente as abas inteligentes de acordo com a demografia da malha
    void atualizarMenuInteligente();
};

#endif // SC_GESTOR_TELAS_M3_H
