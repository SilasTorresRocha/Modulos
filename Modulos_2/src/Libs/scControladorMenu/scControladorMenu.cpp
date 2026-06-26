#include "scControladorMenu.h"

scControladorMenu::scControladorMenu() {
    _logger = nullptr; _display = nullptr; _encoder = nullptr;
    _botaoEnc = nullptr; _reles = nullptr; _watchfaces = nullptr;
    _armazenamento = nullptr;
    
    _estadoAtual = UI_WATCHFACE;
    _nivelAtual = MENU_ROOT;
    _cursorPos = 0;
    _cursorEdicao = 0;
    _releAlvoContexto = 1;
    _watchfaceSelecionada = 0;
    _tsUltimaInteracao = 0;
    _m1Inativo = false; // Presume-se online ou offline, mas nunca Inativo no boot
}

void scControladorMenu::inicializar(scLogger* logger, scGestorDisplay* display, scEncoderRotativo* encoder, scBotaoMultifuncao* botaoEnc, scGestorReles* reles, scWatchfacesM2* watchfaces, scArmazenamentoLocal* armazenamento) {
    _logger = logger; _display = display; _encoder = encoder;
    _botaoEnc = botaoEnc; _reles = reles; _watchfaces = watchfaces;
    _armazenamento = armazenamento;
    
    if (!_logger) return; // Nao falhar silenciosamente (Principio 9)
    if (!_display || !_encoder || !_botaoEnc || !_watchfaces) {
        _logger->erro("CONTROLADOR_MENU", "Falha de injeção! Ponteiros nulos.");
        return;
    }
    
    _carregarPreferencias();
    _tsUltimaInteracao = millis();
}

void scControladorMenu::_carregarPreferencias() {
    if (!_armazenamento) return;
    String val = _armazenamento->obterValor("ui_watchface");
    if (val != "") {
        _watchfaceSelecionada = val.toInt();
        if (_watchfaceSelecionada > 4) _watchfaceSelecionada = 0;
    }
}

void scControladorMenu::_salvarPreferencias() {
    if (!_armazenamento) return;
    _armazenamento->salvarChaveValor("ui_watchface", String(_watchfaceSelecionada));
    // scArmazenamentoLocal já gerencia o commit
}

void scControladorMenu::notificarStatusPeer(String tipo, String status) {
    if (tipo == "M1") {
        _m1Inativo = (status == "inativo");
        if (_logger) _logger->warn("CONTROLADOR_MENU", _m1Inativo ? "Sinalizado: M1 Inativo (Gas Oculto)" : "Sinalizado: M1 Recuperado");
        
        // Se estiver editando Gas quando ele morrer, aborta e recua
        if (_m1Inativo && _nivelAtual == MENU_RELE_OPCOES && _cursorPos == 1) {
            _estadoAtual = UI_WATCHFACE;
        }
    }
}

int scControladorMenu::_obterMaxOpcoes(NivelMenu nivel) {
    switch(nivel) {
        case MENU_ROOT: return 5; // Watchfaces, Rele 1, Rele 2, Agendamentos, Sistema
        case MENU_WATCHFACE_SEL: return 5; // Analogico, Digital, Dash, Simples, Off
        case MENU_RELE_SELECIONAR: return 2; // R1, R2
        case MENU_RELE_OPCOES: 
            return _m1Inativo ? 3 : 4; // Se Inativo esconde Config Gas (que e o 4º item)
        default: return 1;
    }
}

void scControladorMenu::_processarEncoder() {
    int delta = _encoder->getDelta();
    if (delta == 0) return;
    
    _tsUltimaInteracao = millis(); // Reseta timeout
    
    if (_estadoAtual == UI_WATCHFACE) {
        // Qualquer giro de encoder na Watchface entra no menu
        _estadoAtual = UI_MENU;
        _nivelAtual = MENU_ROOT;
        _cursorPos = 0;
        return;
    }
    
    if (_estadoAtual == UI_MENU) {
        _cursorPos += delta;
        int maxOp = _obterMaxOpcoes(_nivelAtual) - 1;
        if (_cursorPos < 0) _cursorPos = maxOp;
        if (_cursorPos > maxOp) _cursorPos = 0;
    } 
    else if (_estadoAtual == UI_EDICAO) {
        _cursorEdicao += delta;
        // Limites da edição dependem do contexto (simplificando por hora)
        if (_cursorEdicao < 0) _cursorEdicao = 0;
        if (_cursorEdicao > 3) _cursorEdicao = 3; 
    }
}

void scControladorMenu::_processarClique() {
    if (_botaoEnc->foiClicado()) {
        _tsUltimaInteracao = millis();
        
        if (_estadoAtual == UI_WATCHFACE) {
            _estadoAtual = UI_MENU;
            _nivelAtual = MENU_ROOT;
            _cursorPos = 0;
        } 
        else if (_estadoAtual == UI_MENU) {
            // Entrar no nivel filho
            switch(_nivelAtual) {
                case MENU_ROOT: _executarAcaoMenuRoot(); break;
                case MENU_WATCHFACE_SEL: _executarAcaoMenuWatchface(); break;
                case MENU_RELE_SELECIONAR: _executarAcaoMenuReleSel(); break;
                case MENU_RELE_OPCOES: _executarAcaoMenuReleOpcoes(); break;
                default: break;
            }
        }
        else if (_estadoAtual == UI_EDICAO) {
            // Confirma edicao
            _estadoAtual = UI_MENU; // Volta pro menu apos confirmar
        }
    }
    
    // Pressao longa volta / aborta
    if (_botaoEnc->foiPressionadoLongo()) {
        _tsUltimaInteracao = millis();
        if (_estadoAtual == UI_EDICAO) {
            _estadoAtual = UI_MENU; // Cancela edicao
        } else if (_estadoAtual == UI_MENU) {
            if (_nivelAtual == MENU_ROOT) {
                _estadoAtual = UI_WATCHFACE; // Sai do menu
            } else {
                // Voltar um nível
                _nivelAtual = MENU_ROOT; // Simplificado para voltar raiz
                _cursorPos = 0;
            }
        }
    }
}

void scControladorMenu::_executarAcaoMenuRoot() {
    switch(_cursorPos) {
        case 0: _nivelAtual = MENU_WATCHFACE_SEL; _cursorPos = 0; break;
        case 1: _nivelAtual = MENU_RELE_OPCOES; _releAlvoContexto = 1; _cursorPos = 0; break;
        case 2: _nivelAtual = MENU_RELE_OPCOES; _releAlvoContexto = 2; _cursorPos = 0; break;
        case 3: _nivelAtual = MENU_AGENDAMENTOS; _cursorPos = 0; break;
        case 4: _nivelAtual = MENU_SISTEMA_INF; _cursorPos = 0; break;
    }
}

void scControladorMenu::_executarAcaoMenuWatchface() {
    _watchfaceSelecionada = _cursorPos;
    _salvarPreferencias();
    _estadoAtual = UI_WATCHFACE; // Volta direto pos aplicar
}

void scControladorMenu::_executarAcaoMenuReleSel() {
    _releAlvoContexto = _cursorPos + 1;
    _nivelAtual = MENU_RELE_OPCOES;
    _cursorPos = 0;
}

void scControladorMenu::_executarAcaoMenuReleOpcoes() {
    // Opcoes do rele: 0=Ligar/Desl, 1=Regra Gas (se visivel), 2=Pós Queda, 3=Reset KWh
    // A logica exata de edicao entrara aqui
    if (_cursorPos == 0) { // Alternar status do relé
        if (_reles) {
            bool est = _reles->getEstadoRele(_releAlvoContexto);
            bool ok = _reles->setEstadoRele(_releAlvoContexto, !est);
            // Se ok for false e EmergenciaGasAtiva for true, o Popup cuida
        }
    } else if (_cursorPos == 1 && !_m1Inativo) { // Regra Gas
        _estadoAtual = UI_EDICAO;
        _cursorEdicao = 0; // Pegaria da leitura real da memoria
    }
}

void scControladorMenu::_verificarTimeout() {
    if (_estadoAtual != UI_WATCHFACE) {
        if (millis() - _tsUltimaInteracao > TEMPO_TIMEOUT_IDLE_MS) {
            _estadoAtual = UI_WATCHFACE; // Voltou pra casa por esquecimento
        }
    }
}

void scControladorMenu::_desenhar() {
    if (!_watchfaces || !_reles) return;
    
    bool gasGlobal = _reles->EmergenciaGasAtiva(1) || _reles->EmergenciaGasAtiva(2);
    
    if (_estadoAtual == UI_WATCHFACE) {
        if (gasGlobal) {
            _watchfaces->desenharAlertaGasTelaCheia(); // Ocupa a tela 100% no idle
        } else {
            _watchfaces->desenharWatchface(_watchfaceSelecionada);
        }
    } else {
        // UI_MENU ou UI_EDICAO
        bool edicao = (_estadoAtual == UI_EDICAO);
        _watchfaces->desenharListaMenu((int)_nivelAtual, _cursorPos, edicao, _m1Inativo, _releAlvoContexto, _cursorEdicao);
        
        if (gasGlobal) {
            _watchfaces->desenharBannerGas(); // Banner sutil, menu continua rodando
        }
    }
}

void scControladorMenu::loop() {
    if (!_display || !_encoder || !_botaoEnc) return; // Nao opera cegamente
    _processarEncoder();
    _processarClique();
    _verificarTimeout();
    _desenhar();
}
