#ifndef SC_GESTOR_DISPLAY_H
#define SC_GESTOR_DISPLAY_H

#include <Arduino.h>

class scGestorDisplay {
public:
    scGestorDisplay();
    void inicializar();
    void atualizar();
    void setPaginaAtual(uint8_t pagina);
    void proximaPagina();

private:
    uint8_t _paginaAtual;
    void desenharWatchFace();
    void desenharInfoCard();
};

#endif // SC_GESTOR_DISPLAY_H
