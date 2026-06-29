#ifndef SC_CONTROLADOR_MENU_H
#define SC_CONTROLADOR_MENU_H

#include <Arduino.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scGestorDisplay/scGestorDisplay.h"
#include "../../LibsGlobais/scEncoderRotativo/scEncoderRotativo.h"
#include "../../LibsGlobais/scBotaoMultifuncao/scBotaoMultifuncao.h"
#include "../scGestorReles/scGestorReles.h"
#include "../scWatchfacesM2/scWatchfacesM2.h"
#include "../../LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"

// Estados da Interface (Máquina de Estados)
enum EstadoInterface {
    UI_WATCHFACE, // Tempo ocioso (Idle)
    UI_MENU,      // Navegando para cima e para baixo nas listas
    UI_EDICAO     // Girando o encoder para alterar o valor de uma variavel selecionada
};

// Níveis de Profundidade da Arvore
enum NivelMenu {
    MENU_ROOT,            // Menu principal
    MENU_WATCHFACE_SEL,   // Selecionar tela idle
    MENU_RELE_SELECIONAR, // Intermediario: Qual rele gerenciar?
    MENU_RELE_OPCOES,     // Opcoes de um rele alvo (Ligar, Retorno, Gas, Reset KWh)
    MENU_RETORNO_QUEDA,   // Submenu para pós-queda de energia
    MENU_REGRA_GAS,       // Submenu para escolher o comportamento em caso de gas
    MENU_AGENDAMENTOS,    // Arvore de Agendamentos (Stub por hora)
    MENU_SISTEMA_INF      // Abas de info tecnica
};

// Timeout para voltar ao relogio sozinho
#define TEMPO_TIMEOUT_IDLE_MS 15000 

class scControladorMenu {
public:
    scControladorMenu();
    void inicializar(scLogger* logger, scGestorDisplay* display, scEncoderRotativo* encoder, scBotaoMultifuncao* botaoEnc, scGestorReles* reles, scWatchfacesM2* watchfaces, scArmazenamentoLocal* armazenamento);
                     
    void loop();
    
    // Interface de Rede para ser injetada pelo .ino quando chegar via MQTT/ESPNOW
    void notificarStatusPeer(String tipo, String status);
    
    // Interface de configuração via MQTT
    void setWatchface(uint8_t id);

private:
    scLogger* _logger;
    scGestorDisplay* _display;
    scEncoderRotativo* _encoder;
    scBotaoMultifuncao* _botaoEnc;
    scGestorReles* _reles;
    scWatchfacesM2* _watchfaces;
    scArmazenamentoLocal* _armazenamento;

    // Variaveis de Estado
    EstadoInterface _estadoAtual;
    NivelMenu _nivelAtual;
    int _cursorPos;            // Ponteiro Y do menu
    int _cursorEdicao;         // Valor que esta sendo alterado durante UI_EDICAO
    uint8_t _releAlvoContexto; // 1 ou 2
    uint8_t _watchfaceSelecionada; // 0=Analog, 1=Digital, 2=Dash, 3=Simples, 4=OFF

    uint32_t _tsUltimaInteracao;
    bool _m1Inativo;
    
    void _carregarPreferencias();
    void _salvarPreferencias();
    
    int _obterMaxOpcoes(NivelMenu nivel);
    void _processarEncoder();
    void _processarClique();
    void _verificarTimeout();
    void _desenhar();
    
    // Tratativas de Clique baseadas no contexto de arvore
    void _executarAcaoMenuRoot();
    void _executarAcaoMenuWatchface();
    void _executarAcaoMenuReleSel();
    void _executarAcaoMenuReleOpcoes();
};

#endif // SC_CONTROLADOR_MENU_H
