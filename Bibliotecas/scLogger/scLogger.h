#ifndef SC_LOGGER_H
#define SC_LOGGER_H

#include <Arduino.h>

enum NivelLog {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

class scLogger {
public:
    scLogger();
    void inicializar(bool outputSerial = true);
    void registrar(NivelLog nivel, String origem, String mensagem);

private:
    bool _outputSerial;
    String formatarMensagem(NivelLog nivel, String origem, String mensagem);
};

#endif // SC_LOGGER_H
