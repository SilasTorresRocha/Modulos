#ifndef SC_AGENDADOR_TAREFAS_H
#define SC_AGENDADOR_TAREFAS_H

#include <Arduino.h>

#define MAX_TAREFAS 10

typedef void (*FuncaoTarefa)();

struct Tarefa {
    FuncaoTarefa funcao;
    uint32_t intervalo;
    uint32_t ultimoTempo;
    bool ativa;
};

class scAgendadorTarefas {
public:
    scAgendadorTarefas();
    bool adicionarTarefa(FuncaoTarefa func, uint32_t intervaloMs);
    void despacharTarefas(); // O motor que gira dentro do loop()

private:
    Tarefa _tarefas[MAX_TAREFAS];
    uint8_t _qtdTarefas;
};

#endif // SC_AGENDADOR_TAREFAS_H
