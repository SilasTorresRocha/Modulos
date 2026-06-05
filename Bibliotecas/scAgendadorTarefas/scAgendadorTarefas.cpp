#include "scAgendadorTarefas.h"

scAgendadorTarefas::scAgendadorTarefas() {
    _qtdTarefas = 0;
    for (int i = 0; i < MAX_TAREFAS; i++) {
        _tarefas[i].ativa = false;
        _tarefas[i].funcao = nullptr;
    }
}

int scAgendadorTarefas::adicionarTarefa(FuncaoTarefa func, uint32_t intervaloMs) {
    if (_qtdTarefas >= MAX_TAREFAS) return -1; // Protecao de Memoria
    
    int id = _qtdTarefas;
    _tarefas[id].funcao = func;
    _tarefas[id].intervalo = intervaloMs;
    // Grava o tempo atual para iniciar a contagem do primeiro ciclo
    _tarefas[id].ultimoTempo = millis(); 
    _tarefas[id].ativa = true;
    
    _qtdTarefas++;
    return id;
}

void scAgendadorTarefas::alterarIntervalo(int idTarefa, uint32_t novoIntervaloMs) {
    if (idTarefa >= 0 && idTarefa < _qtdTarefas) {
        _tarefas[idTarefa].intervalo = novoIntervaloMs;
    }
}

void scAgendadorTarefas::setEstadoTarefa(int idTarefa, bool estado) {
    if (idTarefa >= 0 && idTarefa < _qtdTarefas) {
        _tarefas[idTarefa].ativa = estado;
        
        // Se a tarefa estiver sendo "retomada", o timer deve ser resetado. 
        // Caso contrario ela iria disparar instantaneamente porque o intervalo teria acumulado secretamente durante o tempo em que ela esteve pausada.
        if (estado) {
            _tarefas[idTarefa].ultimoTempo = millis();
        }
    }
}

void scAgendadorTarefas::forcarProximaExecucao(int idTarefa) {
    if (idTarefa >= 0 && idTarefa < _qtdTarefas) {
        // O motor subtrai: (Atual - ultimoTempo). 
        // Ao subtrair o intervalo do Atual, garantimos que o resultado da equacao seja igual ao intervalo, disparando na hora.
        _tarefas[idTarefa].ultimoTempo = millis() - _tarefas[idTarefa].intervalo;
    }
}

void scAgendadorTarefas::despacharTarefas() {
    uint32_t tempoAtual = millis();
    
    for (int i = 0; i < _qtdTarefas; i++) {
        if (_tarefas[i].ativa && _tarefas[i].funcao != nullptr) {
            
            // Protecao contra o Millis Overflow (49 dias) (Redundante, a placa ja deve reinciar a cada 24h evidando Overflow, mas o seguro morreu de velho)
            // Mesmo se o tempoAtual zerar e o ultimoTempo for enorme, a diferenca binaria entre tipos unsigned resultará num tempo real perfeito.
            if ((uint32_t)(tempoAtual - _tarefas[i].ultimoTempo) >= _tarefas[i].intervalo) {
                
                // O relogio bateu. Grava a marca de tempo ANTES de executar a funcao.
                // Se nos gravar DEPOIS da funcao, o tempo gasto dentro da funcao ia ser somado, causando um "desvio/drift" de tempo e a tarefa ia atrasar mais e mais a cada ciclo.
                _tarefas[i].ultimoTempo = tempoAtual;
                
                // Chama o callback (a funcao do .ino injetada)
                _tarefas[i].funcao(); 
            }
        }
    }
}
