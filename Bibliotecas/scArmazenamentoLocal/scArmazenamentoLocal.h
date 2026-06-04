#ifndef SC_ARMAZENAMENTO_LOCAL_H
#define SC_ARMAZENAMENTO_LOCAL_H

#include <Arduino.h>

class scArmazenamentoLocal {
public:
    scArmazenamentoLocal();
    void inicializar();
    bool salvarChaveValor(String chave, String valor);
    String obterValor(String chave);
    bool commitarAlteracoes();

private:
    // Estruturas de gerenciamento e wear leveling
};

#endif // SC_ARMAZENAMENTO_LOCAL_H
