#include "scRadarEcossistemaM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../scGestorBuzzerM3/scGestorBuzzerM3.h"

scRadarEcossistemaM3::scRadarEcossistemaM3() : _logger(nullptr), _buzzer(nullptr), _emergenciaGeralAtiva(false), _ultimoAlarmeTratado(0) {
    // Garante que o Array de Memória fixa comece vazio
    for (int i = 0; i < MAX_NODOS_RADAR; i++) {
        _nodos[i].ocupado = false;
        memset(_nodos[i].mac, 0, TAM_MAC_RADAR);
        memset(_nodos[i].tipo, 0, 4);
    }
}

void scRadarEcossistemaM3::inicializar(scLogger* logger, scGestorBuzzerM3* buzzer) {
    _logger = logger;
    _buzzer = buzzer;
    if (_logger != nullptr) {
        _logger->info("scRadarEcossistema", "Radar Trifasico (Dead Man's Switch) inicializado e pronto para patrulha passiva.");
    }
}

int scRadarEcossistemaM3::_encontrarIndice(const char* mac) {
    for (int i = 0; i < MAX_NODOS_RADAR; i++) {
        if (_nodos[i].ocupado && strncmp(_nodos[i].mac, mac, TAM_MAC_RADAR - 1) == 0) {
            return i;
        }
    }
    return -1;
}

int scRadarEcossistemaM3::_encontrarSlotVazio() {
    for (int i = 0; i < MAX_NODOS_RADAR; i++) {
        if (!_nodos[i].ocupado) {
            return i;
        }
    }
    return -1;
}

void scRadarEcossistemaM3::registrarPing(const char* mac, bool viaEspNow, const char* tipoModulo) {
    int idx = _encontrarIndice(mac);
    
    if (idx == -1) {
        idx = _encontrarSlotVazio();
        if (idx == -1) {
            if (_logger != nullptr) _logger->warn("scRadarEcossistema", "Tabela de Radar estourou (> 20 Nodes). Ignorando patente.");
            return;
        }
        
        // Cadastra um modulo novo que acabou de dar o primeiro oi na rede
        strncpy(_nodos[idx].mac, mac, TAM_MAC_RADAR - 1);
        _nodos[idx].mac[TAM_MAC_RADAR - 1] = '\0';
        
        strncpy(_nodos[idx].tipo, tipoModulo, 3);
        _nodos[idx].tipo[3] = '\0';

        _nodos[idx].ocupado = true;
        _nodos[idx].estadoAtual = VERDE_ONLINE;
        
        if (_logger != nullptr) {
            char buf[64];
            snprintf(buf, sizeof(buf), "Novo No detectado e mapeado pelo Radar: %s", mac);
            _logger->info("scRadarEcossistema", buf);
        }
    }

    // Refresh do batimento cardiaco
    _nodos[idx].ultimoPingMillis = millis();
    _nodos[idx].viaEspNow = viaEspNow;
    
    // Auto-Cura: Módulo que estava morto se reconectou ao Gateway
    if (_nodos[idx].estadoAtual == VERMELHO_MORTO) {
        if (_logger != nullptr) {
            char buf[80];
            snprintf(buf, sizeof(buf), "MILAGRE: No %s RECUPEROU a comunicacao. Ponto Cego sanado.", mac);
            _logger->warn("scRadarEcossistema", buf);
        }
    }
    
    _nodos[idx].estadoAtual = VERDE_ONLINE;
}

void scRadarEcossistemaM3::processar() {
    uint32_t agora = millis();
    bool algumaEmergenciaGeral = false;

    // Rotina passiva: avalia todos os Nós engatados
    for (int i = 0; i < MAX_NODOS_RADAR; i++) {
        if (!_nodos[i].ocupado) continue;

        // Protegido contra overflow de millis() por ser aritmética de unsigned
        uint32_t delta = agora - _nodos[i].ultimoPingMillis;
        EstadoNo estadoVelho = _nodos[i].estadoAtual;
        
        if (delta > 300000) { // 5 minutos de silêncio (300.000 ms)
            _nodos[i].estadoAtual = VERMELHO_MORTO;
            algumaEmergenciaGeral = true; // Derruba a casa toda pra modo Alerta
            
            if (estadoVelho != VERMELHO_MORTO) {
                if (_logger != nullptr) {
                    char buf[90];
                    snprintf(buf, sizeof(buf), "CRITICO: No %s MORTO (> 5 min de silencio). EMERGENCIA DE PONTO CEGO!", _nodos[i].mac);
                    _logger->erro("scRadarEcossistema", buf);
                }
            }
        } 
        else if (delta > 60000) { // 1 minuto de atraso/silêncio (60.000 ms)
            _nodos[i].estadoAtual = AMARELO_FALLBACK;
            
            if (estadoVelho == VERDE_ONLINE) {
                if (_logger != nullptr) {
                    char buf[90];
                    if (_nodos[i].viaEspNow) {
                        snprintf(buf, sizeof(buf), "ALERTA: No %s perdeu Wi-Fi primario (> 60s). Operando na Malha ESP-NOW.", _nodos[i].mac);
                    } else {
                        snprintf(buf, sizeof(buf), "ALERTA: No %s engasgado e perdendo pacotes (> 60s atrasado).", _nodos[i].mac);
                    }
                    _logger->warn("scRadarEcossistema", buf);
                }
            }
        }
    }

    _emergenciaGeralAtiva = algumaEmergenciaGeral;

    // Se a malha estiver furada (alguém Morto - Falha Critica em um sensor de Gas por ex.)
    // O Hub (M3) vai gritar sonoramente pela integridade
    if (_emergenciaGeralAtiva) {
        if (agora - _ultimoAlarmeTratado > 10000) { // Grita a cada 10s (Não prende o loop)
            _ultimoAlarmeTratado = agora;
            if (_buzzer != nullptr) {
                _buzzer->tocarSireneEmergencia();
            }
        }
    }
}

bool scRadarEcossistemaM3::isEmergenciaGeral() const {
    return _emergenciaGeralAtiva;
}

EstadoNo scRadarEcossistemaM3::obterEstado(const char* mac) {
    int idx = _encontrarIndice(mac);
    if (idx != -1) {
        return _nodos[idx].estadoAtual;
    }
    return VERMELHO_MORTO; // Se o despachante pedir de um MAC que não existe, assumimos Dead.
}

bool scRadarEcossistemaM3::existeModuloDoTipo(const char* tipo) const {
    for (int i = 0; i < MAX_NODOS_RADAR; i++) {
        if (_nodos[i].ocupado && strcmp(_nodos[i].tipo, tipo) == 0) {
            // Só retorna true se o nó está online ou no limite Amarelo. 
            // Se estiver morto a mais de 5 dias ignoramos? Nao, se está na tabela, conta.
            // Para UI, basta estar na tabela
            return true;
        }
    }
    return false;
}
