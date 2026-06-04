#ifndef SC_AVISOS_SONOROS_H
#define SC_AVISOS_SONOROS_H

#include <Arduino.h>

enum PadraoSom {
    SILENCIO,
    BIP_CURTO,
    BIP_LONGO,
    SIRENE_EMERGENCIA,   // Som agressivo para falhas críticas (ex: vazamento de gás, pane térmica)
    ALARME_TEMPORIZADOR  // Som rítmico de relógio (ex: timer do forno, despertador diário)
};

class scAvisosSonoros {
public:
    scAvisosSonoros(uint8_t pinoBuzzer);
    void inicializar();
    void tocar(PadraoSom padrao);
    void atualizar(); // Deve ser chamada no loop()

private:
    uint8_t _pino;
    PadraoSom _estadoAtual;
};

#endif // SC_AVISOS_SONOROS_H
