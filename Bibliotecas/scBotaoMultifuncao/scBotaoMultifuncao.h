#ifndef SC_BOTAO_MULTIFUNCAO_H
#define SC_BOTAO_MULTIFUNCAO_H

#include <Arduino.h>

class scBotaoMultifuncao {
public:
    typedef void (*BotaoCallback)();

    scBotaoMultifuncao(uint8_t pino);
    void inicializar();
    void atualizar();

    void setAoClicar(BotaoCallback cb);
    void setAoDuploClique(BotaoCallback cb);
    void setAoSegurar(BotaoCallback cb);

private:
    uint8_t _pino;
    BotaoCallback _cbClique;
    BotaoCallback _cbDuploClique;
    BotaoCallback _cbSegurar;
    // Variaveis de controle de debounce e tempo
};

#endif // SC_BOTAO_MULTIFUNCAO_H
