#include "scGestorReles.h"

// Constantes de Lógica Invertida do Relé (Ativo Baixo)
#define RELE_LIGAR LOW
#define RELE_DESLIGAR HIGH

// Constante de segurança para salvamento periódico do KWh na flash (ex: a cada 1 hora)
#define INTERVALO_SALVAMENTO_KWH_SEG 3600

scGestorReles::scGestorReles() {
    _logger = nullptr;
    _armazenamento = nullptr;
    _relogio = nullptr;
}

void scGestorReles::inicializar(scLogger* logger, scArmazenamentoLocal* armazenamento, scRelogioSincronizado* relogio, uint8_t pinoR1, uint8_t pinoR2) {
    _logger = logger;
    _armazenamento = armazenamento;
    _relogio = relogio;
    
    // Configuraçao Padrão de Memória (evita lixo na RAM)
    _reles[0].pino = pinoR1;
    _reles[1].pino = pinoR2;
    
    for (int i = 0; i < 2; i++) {
        _reles[i].estadoLigado = false;
        _reles[i].bloqueioGasAtivo = false;
        _reles[i].potenciaW = 0.0;
        _reles[i].regraGas = 1;      // Padrão: Ignorar
        _reles[i].retornoPwr = 1;    // Padrão: Sempre OFF
        _reles[i].consumoAcumuladoKWh = 0.0;
        _reles[i].tsUltimoLigamento = 0;
        _reles[i].tsUltimoSalvo = 0;
        
        // Bloqueia fisicamente no nivel DESLIGADO (HIGH) logo de cara para evitar repiques
        pinMode(_reles[i].pino, OUTPUT);
        digitalWrite(_reles[i].pino, RELE_DESLIGAR);
    }
    
    // Busca do banco local (LittleFS/Preferences) os últimos valores salvos
    _carregarConfiguracoesDaFlash();
    
    // Aplica o boot de estado pós-queda
    for (int i = 0; i < 2; i++) {
        uint8_t id = i + 1;
        bool estadoAlvo = false;
        
        if (_reles[i].retornoPwr == 2) {
            estadoAlvo = true; // Sempre Ligar no boot
        } else if (_reles[i].retornoPwr == 3) {
            // Último estado (já lido no _carregarConfiguracoesDaFlash)
            estadoAlvo = _reles[i].estadoLigado;
        } else {
            estadoAlvo = false; // Sempre OFF (1)
        }
        
        if (_logger) _logger->info("GESTOR_RELES", "Boot Rele " + String(id) + " - Aplicando estado: " + String(estadoAlvo ? "ON" : "OFF"));
        
        // Força a aplicação ignorando travas (pois é o boot)
        _reles[i].estadoLigado = estadoAlvo;
        _aplicarEstadoFisico(id);
        
        // Inicia âncora de tempo se atracou ligado
        if (estadoAlvo && _relogio && _relogio->estaSincronizado()) {
            _reles[i].tsUltimoLigamento = _relogio->obterHoraUnix();
        }
    }
}

void scGestorReles::_carregarConfiguracoesDaFlash() {
    if (!_armazenamento) {
        if (_logger) _logger->erro("GESTOR_RELES", "Ponteiro de Armazenamento NULO ao carregar config!");
        return;
    }
    
    for (int i = 0; i < 2; i++) {
        String releKey = "r" + String(i + 1) + "_";
        
        // Retorno Pós-Queda
        String val = _armazenamento->obterValor(releKey + "retpwr");
        if (val != "") _reles[i].retornoPwr = val.toInt();
        
        // Potência
        val = _armazenamento->obterValor(releKey + "potw");
        if (val != "") _reles[i].potenciaW = val.toFloat();
        
        // Regra de Gás
        val = _armazenamento->obterValor(releKey + "fbgas");
        if (val != "") _reles[i].regraGas = val.toInt();
        
        // Último Estado
        val = _armazenamento->obterValor(releKey + "ultst");
        if (val != "") _reles[i].estadoLigado = (val == "1");
        
        // Consumo Acumulado
        val = _armazenamento->obterValor(releKey + "totkw");
        if (val != "") _reles[i].consumoAcumuladoKWh = val.toFloat();
    }
}

void scGestorReles::_salvarEstadoPersistente(uint8_t id_rele) {
    if (!_armazenamento) {
        if (_logger) _logger->erro("GESTOR_RELES", "Ponteiro de Armazenamento NULO ao salvar estado!");
        return;
    }
    int i = _idx(id_rele);
    String key = "r" + String(id_rele) + "_ultst";
    _armazenamento->salvarChaveValor(key, _reles[i].estadoLigado ? "1" : "0");
    _armazenamento->commitarAlteracoes(); // Nao obrigatorio dependendo da lib, mas o Armazenamento já se resolve
}

void scGestorReles::_salvarConsumoPersistente(uint8_t id_rele) {
    if (!_armazenamento) {
        if (_logger) _logger->erro("GESTOR_RELES", "Ponteiro de Armazenamento NULO ao salvar consumo!");
        return;
    }
    int i = _idx(id_rele);
    String key = "r" + String(id_rele) + "_totkw";
    _armazenamento->salvarChaveValor(key, String(_reles[i].consumoAcumuladoKWh, 4));
    _armazenamento->commitarAlteracoes(); // Nao obrigatorio dependendo da lib, mas o Armazenamento já se resolve
}

void scGestorReles::_aplicarEstadoFisico(uint8_t id_rele) {
    int i = _idx(id_rele);
    if (_reles[i].estadoLigado) {
        digitalWrite(_reles[i].pino, RELE_LIGAR);
    } else {
        digitalWrite(_reles[i].pino, RELE_DESLIGAR);
    }
}

bool scGestorReles::setEstadoRele(uint8_t id_rele, bool ligar) {
    int i = _idx(id_rele);
    
    // Trava de Contingência
    if (_reles[i].bloqueioGasAtivo) {
        if (_logger) _logger->warn("GESTOR_RELES", "Acao bloqueada no Rele " + String(id_rele) + " devido a Emergencia de Gas!");
        return false;
    }
    
    if (_reles[i].estadoLigado == ligar) return true; // Já está no estado
    
    // Se estava ligado e agora vai desligar, calcula e salva o consumo do período
    if (_reles[i].estadoLigado && !ligar) {
        if (_relogio && _relogio->estaSincronizado() && _reles[i].tsUltimoLigamento > 0) {
            uint32_t agora = _relogio->obterHoraUnix();
            float horasDecorridas = (agora - _reles[i].tsUltimoLigamento) / 3600.0;
            float consumoSessao = (_reles[i].potenciaW / 1000.0) * horasDecorridas;
            _reles[i].consumoAcumuladoKWh += consumoSessao;
            _salvarConsumoPersistente(id_rele);
        }
        _reles[i].tsUltimoLigamento = 0; // Zera a âncora
    }
    
    // Se estava desligado e vai ligar, cria a âncora de tempo
    if (!_reles[i].estadoLigado && ligar) {
        if (_relogio && _relogio->estaSincronizado()) {
            _reles[i].tsUltimoLigamento = _relogio->obterHoraUnix();
            _reles[i].tsUltimoSalvo = _reles[i].tsUltimoLigamento;
        }
    }
    
    // Aplica as mudanças
    _reles[i].estadoLigado = ligar;
    _aplicarEstadoFisico(id_rele);
    
    // Persiste o último estado caso a energia caia agora
    if (_reles[i].retornoPwr == 3) {
        _salvarEstadoPersistente(id_rele);
    }
    
    if (_logger) _logger->info("GESTOR_RELES", "Rele " + String(id_rele) + " alterado para: " + String(ligar ? "ON" : "OFF"));
    return true;
}

bool scGestorReles::getEstadoRele(uint8_t id_rele) {
    return _reles[_idx(id_rele)].estadoLigado;
}

void scGestorReles::configurarRele(uint8_t id_rele, float potenciaW, uint8_t regraGas, uint8_t retornoPwr) {
    int i = _idx(id_rele);
    _reles[i].potenciaW = potenciaW;
    _reles[i].regraGas = regraGas;
    _reles[i].retornoPwr = retornoPwr;
    
    if (_armazenamento) {
        String releKey = "r" + String(id_rele) + "_";
        _armazenamento->salvarChaveValor(releKey + "potw", String(potenciaW, 2));
        _armazenamento->salvarChaveValor(releKey + "fbgas", String(regraGas));
        _armazenamento->salvarChaveValor(releKey + "retpwr", String(retornoPwr));
        if (_logger) _logger->info("GESTOR_RELES", "Configuracoes gravadas na flash para Rele " + String(id_rele));
    }
}

void scGestorReles::resetarConsumo(uint8_t id_rele) {
    int i = _idx(id_rele);
    _reles[i].consumoAcumuladoKWh = 0.0;
    
    // Se estiver ligado, reseta a âncora para não considerar o passado na próxima conta
    if (_reles[i].estadoLigado && _relogio && _relogio->estaSincronizado()) {
        _reles[i].tsUltimoLigamento = _relogio->obterHoraUnix();
        _reles[i].tsUltimoSalvo = _reles[i].tsUltimoLigamento;
    }
    
    _salvarConsumoPersistente(id_rele);
    if (_logger) _logger->info("GESTOR_RELES", "Consumo do Rele " + String(id_rele) + " foi RESETADO!");
}

float scGestorReles::getConsumoKWh(uint8_t id_rele) {
    int i = _idx(id_rele);
    float retornoAcumulado = _reles[i].consumoAcumuladoKWh;
    
    // Se estiver ligado neste exato milissegundo, adiciona o delta provisório
    if (_reles[i].estadoLigado && _relogio && _relogio->estaSincronizado() && _reles[i].tsUltimoLigamento > 0) {
        uint32_t agora = _relogio->obterHoraUnix();
        if (agora > _reles[i].tsUltimoLigamento) {
            float horasDecorridas = (agora - _reles[i].tsUltimoLigamento) / 3600.0;
            float consumoProvisorio = (_reles[i].potenciaW / 1000.0) * horasDecorridas;
            retornoAcumulado += consumoProvisorio;
        }
    }
    return retornoAcumulado;
}

uint32_t scGestorReles::getTempoLigadoSessao(uint8_t id_rele) {
    int i = _idx(id_rele);
    if (!_reles[i].estadoLigado || !_relogio || !_relogio->estaSincronizado() || _reles[i].tsUltimoLigamento == 0) {
        return 0;
    }
    return _relogio->obterHoraUnix() - _reles[i].tsUltimoLigamento;
}

void scGestorReles::acionarEmergenciaGas(bool vazamentoDetectado) {
    if (_logger) _logger->warn("GESTOR_RELES", vazamentoDetectado ? "EMERGENCIA GAS DECLARADA!" : "EMERGENCIA GAS ENCERRADA.");
    
    for (int i = 0; i < 2; i++) {
        uint8_t id = i + 1;
        if (vazamentoDetectado) {
            if (_reles[i].regraGas == 2) { // Bloquear/Desligar
                if (_logger) _logger->warn("GESTOR_RELES", "Cortando forcadamente Rele " + String(id));
                // Chama a mudança mas ignora a trava (passando ela para false antes temporariamente)
                _reles[i].bloqueioGasAtivo = false;
                setEstadoRele(id, false); 
                _reles[i].bloqueioGasAtivo = true; // Trava o relé no OFF
                
            } else if (_reles[i].regraGas == 3) { // Forçar Exaustor
                if (_logger) _logger->warn("GESTOR_RELES", "Ligando forcadamente Exaustor no Rele " + String(id));
                _reles[i].bloqueioGasAtivo = false;
                setEstadoRele(id, true);
                _reles[i].bloqueioGasAtivo = true; // Trava o relé no ON
            }
        } else {
            // Fim do vazamento. Libera a trava. (Os estados ficam como estavam durante a emergência, 
            // caberá ao usuário ou aos agendamentos futuros alterar).
            _reles[i].bloqueioGasAtivo = false;
        }
    }
}

bool scGestorReles::EmergenciaGasAtiva(uint8_t id_rele) {
    return _reles[_idx(id_rele)].bloqueioGasAtivo;
}

void scGestorReles::loop() {
    if (!_relogio || !_relogio->estaSincronizado()) return;
    
    uint32_t agora = _relogio->obterHoraUnix();
    
    // Backup periódico da memória de consumo para não perder em caso de queda de energia
    for (int i = 0; i < 2; i++) {
        if (_reles[i].estadoLigado && _reles[i].tsUltimoLigamento > 0) {
            // Se passou mais de X tempo desde o último save parcial
            if (agora - _reles[i].tsUltimoSalvo >= INTERVALO_SALVAMENTO_KWH_SEG) {
                
                // Consolida o trecho na memória real
                float horasDecorridas = (agora - _reles[i].tsUltimoLigamento) / 3600.0;
                float consumoParcial = (_reles[i].potenciaW / 1000.0) * horasDecorridas;
                
                _reles[i].consumoAcumuladoKWh += consumoParcial;
                _salvarConsumoPersistente(i + 1);
                
                // Reinicia a âncora para o tempo de agora
                _reles[i].tsUltimoLigamento = agora;
                _reles[i].tsUltimoSalvo = agora;
                
                if (_logger) _logger->info("GESTOR_RELES", "Backup de KWh parcial realizado via loop para Rele " + String(i + 1));
            }
        }
    }
}
