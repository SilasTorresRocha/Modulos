#ifndef SC_GESTOR_AGENDAMENTOS_H
#define SC_GESTOR_AGENDAMENTOS_H

#include <Arduino.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"
#include "../../LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "../scGestorReles/scGestorReles.h"

struct Agendamento {
    uint8_t id_agd;
    uint8_t rele;
    uint8_t diaSemana; // 0=DOM, ..., 6=SAB, 8=TODOS
    uint8_t hora;
    uint8_t min;
    bool acao_ligar;
    bool ativo;
    bool executado_hoje; // Flag anti-metralhadora do loop()
};

class scGestorAgendamentos {
public:
    scGestorAgendamentos();
    void inicializar(scLogger* logger, scRelogioSincronizado* relogio, scArmazenamentoLocal* armazenamento, scGestorReles* reles);
    
    // Motor Otimizado que deve ir no loop principal
    void loop();
    
    // Interface de Criação/Cancelamento para ser usada pela Tela OLED ou via MQTT Web
    bool adicionarAgendamento(uint8_t id, uint8_t rele, String diaStr, uint8_t hora, uint8_t min, bool ligar);
    bool excluirAgendamento(uint8_t id);
    
    // Retorna o ponteiro para a camada de Interface desenhar a lista de Agendamentos na tela
    Agendamento* listarAgendamentos(); 

private:
    scLogger* _logger;
    scRelogioSincronizado* _relogio;
    scArmazenamentoLocal* _armazenamento;
    scGestorReles* _reles;
    
    Agendamento _agendamentos[10];
    int _ultimoMinutoChecado;
    
    void _carregarDaMemoria();
    void _salvarNaMemoria();
    uint8_t _converterDiaParaInt(String diaStr);
};

#endif // SC_GESTOR_AGENDAMENTOS_H
