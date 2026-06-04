#ifndef SC_ENCODER_ROTATIVO_H
#define SC_ENCODER_ROTATIVO_H

#include <Arduino.h>

class scEncoderRotativo {
public:
    scEncoderRotativo(uint8_t pinoCLK, uint8_t pinoDT);
    void inicializar();
    long obterContagem();
    void resetarContagem();

private:
    uint8_t _pinoCLK;
    uint8_t _pinoDT;
    volatile long _contagem;
    
    // A ISR (Interrupt Service Routine) deve ser statica e estar no IRAM do ESP
    static void IRAM_ATTR isrEncoder();
    
    // Instancia singleton provisoria para a ISR acessar variaveis de classe
    static scEncoderRotativo* _instancia;
};

#endif // SC_ENCODER_ROTATIVO_H
