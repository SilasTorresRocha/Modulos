#include "scGestorBuzzerM3.h"
#include "../../LibsGlobais/scLogger/scLogger.h"

// Repasse de construtor travado na memoria (Zero Heap Allocation)
scGestorBuzzerM3::scGestorBuzzerM3(uint8_t pinoBuzzer) : _avisos(pinoBuzzer), _logger(nullptr) {
}

void scGestorBuzzerM3::inicializar(scLogger* logger) {
    _logger = logger;
    _avisos.inicializar();
    
    if (_logger != nullptr) {
        _logger->info("scGestorBuzzerM3", "Gestor de audio (Wrapper) inicializado com sucesso.");
    }
}

void scGestorBuzzerM3::tocarBipUI() {
    _avisos.tocar(BIP_CURTO);
}

void scGestorBuzzerM3::tocarSireneEmergencia() {
    if (_logger != nullptr) {
        _logger->warn("scGestorBuzzerM3", "Iniciando SIRENE DE EMERGENCIA!");
    }
    _avisos.tocar(SIRENE_EMERGENCIA);
}

void scGestorBuzzerM3::tocarSucesso() {
    _avisos.tocar(BIP_DUPLO);
}

void scGestorBuzzerM3::setMute(bool mutado) {
    _avisos.setMute(mutado);
    if (_logger != nullptr) {
        String estado = mutado ? "ATIVADO (Bips Silenciados)" : "DESATIVADO (Sons Normais)";
        _logger->info("scGestorBuzzerM3", "Mute Global: " + estado);
    }
}

bool scGestorBuzzerM3::isMutado() {
    return _avisos.Mutado();
}

void scGestorBuzzerM3::atualizar() {
    // Roda a maquina de estados interna sem bloquear o processador
    _avisos.atualizar();
}
