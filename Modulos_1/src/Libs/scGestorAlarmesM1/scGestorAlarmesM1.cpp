#include "scGestorAlarmesM1.h"

scGestorAlarmesM1::scGestorAlarmesM1() {
    _logger = nullptr;
    _avisos = nullptr;
    
    // Tempos de default (Fallback caso o backend não tenha enviado via MQTT)
    _almPrepSegundos = 1800; // 30 minutos
    _almCritSegundos = 3600; // 1 hora
    
    _fornoLigado = false;
    _vazamentoGas = false;
    _inicioFornoMillis = 0;
    
    _alarmeAtual = SILENCIO;
}

void scGestorAlarmesM1::inicializar(scLogger* logger, scAvisosSonoros* avisosSonoros) {
    _logger = logger;
    _avisos = avisosSonoros;

    if (!_logger || !_avisos) {
        // Princípio 10: Fail-Fast por ausência de dependências estruturais
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
            delay(100);
            yield();
        }
    }

    _logger->info("ALM_M1", "Gestor de Alarmes inicializado com sucesso.");
}

void scGestorAlarmesM1::setAlarmes(uint32_t preparoSegundos, uint32_t criticoSegundos) {
    if (_almPrepSegundos != preparoSegundos || _almCritSegundos != criticoSegundos) {
        _almPrepSegundos = preparoSegundos;
        _almCritSegundos = criticoSegundos;
        _logger->info("ALM_M1", "Limites atualizados -> Preparo: " + String(_almPrepSegundos) + "s, Critico: " + String(_almCritSegundos) + "s");
    }
}

void scGestorAlarmesM1::setEstadoForno(bool ligado) {
    if (_fornoLigado != ligado) {
        _fornoLigado = ligado;
        if (_fornoLigado) {
            _inicioFornoMillis = millis(); // Marca o momento exato da ignição
        } else {
            _inicioFornoMillis = 0; // Previne calculo errado de uptime
        }
    }
}

void scGestorAlarmesM1::setEstadoVazamento(bool vazando) {
    _vazamentoGas = vazando;
}

void scGestorAlarmesM1::processar() {
    // Resolve a hierarquia do som desejado (Ameaça maior esmaga alerta menor)
    PadraoSom alarmeDesejado = SILENCIO;

    if (_vazamentoGas) {
        // Prioridade 1: Risco Iminente
        alarmeDesejado = SIRENE_EMERGENCIA;
    } else if (_fornoLigado) {
        // Prioridade 2: Cálculos de limites do Forno
        uint32_t uptimeSegundos = (millis() - _inicioFornoMillis) / 1000;

        // Alarmes configurados com 0 (zero) desativam a feature
        if (_almCritSegundos > 0 && uptimeSegundos >= _almCritSegundos) {
            alarmeDesejado = SIRENE_EMERGENCIA;
        } else if (_almPrepSegundos > 0 && uptimeSegundos >= _almPrepSegundos) {
            alarmeDesejado = ALARME_TEMPORIZADOR;
        }
    }

    // Compara com o Hardware de Áudio para economizar requisições
    if (_alarmeAtual != alarmeDesejado) {
        _alarmeAtual = alarmeDesejado;
        
        // Pede a Global scAvisosSonoros para tomar controle do Buzzer
        _avisos->tocar(_alarmeAtual);

        // Dispara logs audíveis para diagnóstico do servidor
        if (_alarmeAtual == SIRENE_EMERGENCIA) {
            _logger->erro("ALM_M1", "DISPARO: Sirene de Emergencia Ativada (Vazamento ou Limite Critico do Forno)!");
        } else if (_alarmeAtual == ALARME_TEMPORIZADOR) {
            _logger->info("ALM_M1", "DISPARO: Temporizador de Preparo ativado (Forno).");
        } else if (_alarmeAtual == SILENCIO) {
            _logger->info("ALM_M1", "Ameaca cessada. Alarmes silenciados.");
        }
    }
}

uint32_t scGestorAlarmesM1::TempoForno() const {
    if (_fornoLigado) {
        return (millis() - _inicioFornoMillis) / 1000;
    }
    return 0; // Forno desligado não tem tempo rodando
}

uint32_t scGestorAlarmesM1::AlmPrep() const {
    return _almPrepSegundos;
}

uint32_t scGestorAlarmesM1::AlmCrit() const {
    return _almCritSegundos;
}
