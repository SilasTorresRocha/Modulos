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

void scControladorMenu::setWatchface(uint8_t id) {
    if (id > 4) id = 0;
    _watchfaceSelecionada = id;
    _salvarPreferencias();
    if (_logger) _logger->info("CONTROLADOR_MENU", "Watchface alterada via MQTT para: " + String(id));
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
        case MENU_ROOT: return 6; // Watchfaces, Rele 1, Rele 2, Agendamentos, Sistema, Voltar
        case MENU_WATCHFACE_SEL: return 6; // Analogico, Digital, Dash, Simples, Off, Voltar
        case MENU_RELE_SELECIONAR: return 3; // R1, R2, Voltar
        case MENU_RELE_OPCOES: 
            return _m1Inativo ? 5 : 6; // Ligar, Potencia, (Gas), Retorno, Reset, Voltar
        case MENU_RETORNO_QUEDA: return 4; // Off, On, Ultimo, Voltar
        case MENU_REGRA_GAS: return 4; // Ignorar, Desligar, Exaustor, Voltar
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
        // Se a gente ta editando a Potencia (cursorPos == 1 na tela RELE_OPCOES)
        if (_nivelAtual == MENU_RELE_OPCOES && _cursorPos == 1) {
            _cursorEdicao += (delta * 50); // Incrementa de 50W em 50W
            if (_cursorEdicao < 0) _cursorEdicao = 0;
            if (_cursorEdicao > 3500) _cursorEdicao = 3500;
        } else {
            _cursorEdicao += delta;
            if (_cursorEdicao < 0) _cursorEdicao = 0;
            if (_cursorEdicao > 3) _cursorEdicao = 3; 
        }
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
            int maxOp = _obterMaxOpcoes(_nivelAtual) - 1;
            
            if (_nivelAtual == MENU_SISTEMA_INF) {
                // Tela INF não é um menu comum, clicar nela sempre volta!
                _estadoAtual = UI_MENU;
                _nivelAtual = MENU_ROOT;
                _cursorPos = 0;
            }
            else if (_cursorPos == maxOp) { 
                // A ultima opcao de QUALQUER menu é sempre o "Voltar"
                if (_nivelAtual == MENU_ROOT) {
                    _estadoAtual = UI_WATCHFACE;
                } else {
                    _nivelAtual = MENU_ROOT;
                    _cursorPos = 0;
                }
            } else {
                switch(_nivelAtual) {
                    case MENU_ROOT: _executarAcaoMenuRoot(); break;
                    case MENU_WATCHFACE_SEL: _executarAcaoMenuWatchface(); break;
                    case MENU_RELE_SELECIONAR: _executarAcaoMenuReleSel(); break;
                    case MENU_RELE_OPCOES: _executarAcaoMenuReleOpcoes(); break;
                    case MENU_AGENDAMENTOS: 
                        // Implementacao futura: Criar menu complexo de hora/dia. 
                        // Por hora, recua para a raiz.
                        _estadoAtual = UI_MENU;
                        _nivelAtual = MENU_ROOT;
                        _cursorPos = 0;
                        break;
                    case MENU_REGRA_GAS: // Salva regra e volta
                        if (_reles) {
                            if (_cursorPos < 3) _reles->setRegraGas(_releAlvoContexto, _cursorPos + 1);
                        }
                        _estadoAtual = UI_MENU;
                        _nivelAtual = MENU_RELE_OPCOES;
                        _cursorPos = 0;
                        break;
                    case MENU_RETORNO_QUEDA: // Salva retorno e volta
                        if (_reles) {
                            if (_cursorPos < 3) _reles->setRetornoPwr(_releAlvoContexto, _cursorPos + 1);
                        }
                        _estadoAtual = UI_MENU;
                        _nivelAtual = MENU_RELE_OPCOES;
                        _cursorPos = 0;
                        break;
                    default: break;
                }
            }
        }
        else if (_estadoAtual == UI_EDICAO) {
            // Confirma edicao
            if (_nivelAtual == MENU_RELE_OPCOES && _cursorPos == 1) {
                if (_reles) _reles->setPotenciaW(_releAlvoContexto, _cursorEdicao);
            }
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
    // Opcoes: 0=Ligar/Desl, 1=Potencia, 2=Regra Gas(se visivel), 3=Pos Queda, 4=Reset KWh
    int offset = _m1Inativo ? 1 : 0;
    
    if (_cursorPos == 0) { // Alternar status do relé
        if (_reles) {
            bool est = _reles->getEstadoRele(_releAlvoContexto);
            bool ok = _reles->setEstadoRele(_releAlvoContexto, !est);
        }
    } else if (_cursorPos == 1) { // Potencia
        _estadoAtual = UI_EDICAO;
        _cursorEdicao = _reles ? _reles->getPotenciaW(_releAlvoContexto) : 0;
    } else if (!_m1Inativo && _cursorPos == 2) { // Regra Gas
        _estadoAtual = UI_MENU;
        _nivelAtual = MENU_REGRA_GAS;
        _cursorPos = _reles ? (_reles->getRegraGas(_releAlvoContexto) - 1) : 0;
        if (_cursorPos < 0) _cursorPos = 0;
    } else if (_cursorPos == (3 - offset)) { // Retorno Queda
        _estadoAtual = UI_MENU;
        _nivelAtual = MENU_RETORNO_QUEDA;
        _cursorPos = _reles ? (_reles->getRetornoPwr(_releAlvoContexto) - 1) : 0;
        if (_cursorPos < 0) _cursorPos = 0;
    } else if (_cursorPos == (4 - offset)) { // Resetar Consumo
        if (_reles) _reles->resetarConsumo(_releAlvoContexto);
        _estadoAtual = UI_WATCHFACE; // Volta para o relogio apos zerar
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

    U8G2* u8g2 = _display->getU8G2();
    if (u8g2) u8g2->clearBuffer();
    
    _desenhar();
    
    if (u8g2) u8g2->sendBuffer();
}
