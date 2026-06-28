#ifndef SC_DESPACHANTE_COMANDOS_M3_H
#define SC_DESPACHANTE_COMANDOS_M3_H

#include <Arduino.h>
#include <ArduinoJson.h>

class scLogger;
class scGestorBuzzerM3;
class scGestorDispositivosM3;

class scDespachanteComandosM3 {
private:
    scLogger* _logger;
    scGestorBuzzerM3* _buzzer;
    scGestorDispositivosM3* _gestorDispositivos;
    const char* _meuMac;

public:
    scDespachanteComandosM3();

    // Injeta os componentes de HW que sofrem acoes diretas (Efeitos Colaterais)
    void inicializar(const char* meuMac, scLogger* logger, scGestorBuzzerM3* buzzer, scGestorDispositivosM3* gestorDispositivos);

    // Método central. Recebe o pacote Json cru do Controlador de Tráfego e, usando 
    // StaticJsonDocument, verifica se o pacote é endereçado a este módulo.
    void processarPacote(const char* payloadJson);
};

#endif // SC_DESPACHANTE_COMANDOS_M3_H
