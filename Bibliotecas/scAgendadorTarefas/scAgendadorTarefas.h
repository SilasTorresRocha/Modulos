#ifndef SC_AGENDADOR_TAREFAS_H
#define SC_AGENDADOR_TAREFAS_H

#include <Arduino.h>

#define MAX_TAREFAS 10

// O Agendador opera via Ponteiro de Funcao (Callback). 
// Isso mantem o motor desacoplado do arquivo .ino
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
    
    // Adiciona uma tarefa e retorna o ID unico dela (para alteracoes dinamicas futuras)
    // Retorna -1 caso o vetor esteja lotado
    int adicionarTarefa(FuncaoTarefa func, uint32_t intervaloMs);
    
    // Permite mudar o intervalo de uma tarefa em tempo de execucao
    // Ex: Mudar o piscar do LED de 1000ms para 100ms se o Wi-Fi cair
    void alterarIntervalo(int idTarefa, uint32_t novoIntervaloMs);
    
    // Pausa ou Retoma o loop de uma tarefa especifica
    void setEstadoTarefa(int idTarefa, bool estado);
    
    // Antecipa e forca a tarefa a rodar na proxima volta do loop() ignorando a espera
    void forcarProximaExecucao(int idTarefa);
    
    // O motor de batimento cardiaco. Deve ser a unica coisa rodando solta dentro do loop() do Módulo
    void despacharTarefas(); 

private:
    Tarefa _tarefas[MAX_TAREFAS];
    uint8_t _qtdTarefas;
};

#endif // SC_AGENDADOR_TAREFAS_H
