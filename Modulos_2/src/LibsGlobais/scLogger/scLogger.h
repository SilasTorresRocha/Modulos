#ifndef SC_LOGGER_H
#define SC_LOGGER_H

#include <Arduino.h>

class scRelogioSincronizado;
class scArmazenamentoLocal;
class scMQTTLib;

enum NivelLog {
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR
};

class scLogger {
public:
    scLogger();
    
    // Injecao de dependencias para timestamp e fallback de erros
    void inicializar(bool outputSerial = true, 
                     scRelogioSincronizado* relogio = nullptr, 
                     scArmazenamentoLocal* armazenamento = nullptr, 
                     scMQTTLib* mqtt = nullptr);
                     
    void registrar(NivelLog nivel, String origem, String mensagem);
    
    // Varre a memoria fisica no boot e envia os logs de crash anteriores
    void despacharErrosPendentes();

    // Helpers para facilitar a chamada
    void info(String origem, String mensagem);
    void warn(String origem, String mensagem);
    void erro(String origem, String mensagem);

private:
    bool _outputSerial;
    scRelogioSincronizado* _relogio;
    scArmazenamentoLocal* _armazenamento;
    scMQTTLib* _mqtt;

    String formatarMensagem(NivelLog nivel, String origem, String mensagem);
};

#endif // SC_LOGGER_H
