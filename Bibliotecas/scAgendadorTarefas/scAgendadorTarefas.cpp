#include "scAgendadorTarefas.h"

scAgendadorTarefas::scAgendadorTarefas() : _qtdTarefas(0) {
    for (int i = 0; i < MAX_TAREFAS; i++) {
        _tarefas[i].ativa = false;
    }
}

bool scAgendadorTarefas::adicionarTarefa(FuncaoTarefa func, uint32_t intervaloMs) {
    if (_qtdTarefas >= MAX_TAREFAS) return false;
    
    _tarefas[_qtdTarefas].funcao = func;
    _tarefas[_qtdTarefas].intervalo = intervaloMs;
    _tarefas[_qtdTarefas].ultimoTempo = millis();
    _tarefas[_qtdTarefas].ativa = true;
    _qtdTarefas++;
    
    return true;
}

void scAgendadorTarefas::despacharTarefas() {
    uint32_t tempoAtual = millis();
    
    for (int i = 0; i < _qtdTarefas; i++) {
        if (_tarefas[i].ativa && (tempoAtual - _tarefas[i].ultimoTempo >= _tarefas[i].intervalo)) {
            _tarefas[i].ultimoTempo = tempoAtual;
            _tarefas[i].funcao(); // Executa a tarefa
        }
    }
}
