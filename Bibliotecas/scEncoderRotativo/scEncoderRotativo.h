#ifndef SC_ENCODER_ROTATIVO_H
#define SC_ENCODER_ROTATIVO_H

#include <Arduino.h>

class scEncoderRotativo {
public:
    scEncoderRotativo(uint8_t pinoCLK, uint8_t pinoDT);
    
    void inicializar();
    
    // Obtem a contagem atual (Protegido contra condicao de corrida / Race Condition)
    long obterContagem();
    
    // Zera a contagem 
    void resetarContagem();

private:
    uint8_t _pinoCLK;
    uint8_t _pinoDT;
    
    // 'volatile' força o compilador a ler a RAM toda vez, 
    // pois a variavel pode mudar "magicamente" por causa da ISR a qualquer microsegundo.
    volatile long _contagem;
    
    // Armazena o estado do Gray Code
    volatile uint8_t _estadoAnterior;
    
    // A ISR (Interrupt Service Routine) deve ser estatica e estar na Instruction RAM (IRAM)
    static void IRAM_ATTR isrEncoder();
    
    // Instancia singleton para a ISR estatica conseguir acessar as variaveis membro da classe
    static scEncoderRotativo* _instancia;
};

#endif // SC_ENCODER_ROTATIVO_H
