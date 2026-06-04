#ifndef SC_TELEMETRIA_BASE_H
#define SC_TELEMETRIA_BASE_H

#include <Arduino.h>

class scTelemetriaBase {
public:
    scTelemetriaBase();
    void inicializar();
    void construirHeartbeatBase();
    void adicionarDado(String chave, String valor);
    void enviar();

private:
    String _jsonAtual;
};

#endif // SC_TELEMETRIA_BASE_H
