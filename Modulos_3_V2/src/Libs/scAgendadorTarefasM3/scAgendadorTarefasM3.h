#ifndef SC_AGENDADOR_TAREFAS_M3_H
#define SC_AGENDADOR_TAREFAS_M3_H

#include <Arduino.h>

// ==========================================
// Módulos Globais (Nuvem, EEPROM e Logger)
// ==========================================
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "../../LibsGlobais/scGestorRede/scGestorRede.h"
#include "../../LibsGlobais/scMQTTLib/scMQTTLib.h"
#include "../../LibsGlobais/scConfigOTA/scConfigOTA.h"

// ==========================================
// Módulos Nativos do Hub (M3)
// ==========================================
#include "../scRTCFisicoM3/scRTCFisicoM3.h"
#include "../scGestorBuzzerM3/scGestorBuzzerM3.h"
#include "../scGestorSDCardM3/scGestorSDCardM3.h"
#include "../scGestorDispositivosM3/scGestorDispositivosM3.h"
#include "../scMonitorSaudeM3/scMonitorSaudeM3.h"
#include "../scRadarEcossistemaM3/scRadarEcossistemaM3.h"
#include "../scRoteadorBridgeM3/scRoteadorBridgeM3.h"
#include "../scDespachanteComandosM3/scDespachanteComandosM3.h"
#include "../scTelemetriaM3/scTelemetriaM3.h"
#include "../scMotorGraficoM3/scMotorGraficoM3.h"
#include "../scGestorSlideshowM3/scGestorSlideshowM3.h"
#include "../scGestorTelasM3/scGestorTelasM3.h"

class scAgendadorTarefasM3 {
private:
    scArmazenamentoLocal _eeprom;
    scGestorRede _rede;
    scMQTTLib _mqtt;
    scConfigOTA _ota;

    scLogger _logger;
    scRTCFisicoM3 _rtc;
    scGestorBuzzerM3 _buzzer;
    scGestorSDCardM3 _sdCard;
    scGestorDispositivosM3 _gestorDispositivos;
    scMonitorSaudeM3 _saude;
    scRadarEcossistemaM3 _radar;
    scRoteadorBridgeM3 _roteador;
    scDespachanteComandosM3 _despachante;
    scTelemetriaM3 _telemetria;
    scMotorGraficoM3 _motorGrafico;
    scGestorSlideshowM3 _slideshow;
    scGestorTelasM3 _gestorTelas;

public:
    scAgendadorTarefasM3();

    // Inicializa todos os módulos injetando as dependências corretas (O Maestro)
    void inicializar();

    // Loop estrito Bare-Metal. A ordem da execução é rigidamente controlada aqui.
    void processar();
};

#endif // SC_AGENDADOR_TAREFAS_M3_H
