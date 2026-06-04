#ifndef SC_DESPACHANTE_COMANDOS_H
#define SC_DESPACHANTE_COMANDOS_H

#include <Arduino.h>

class scGestorRede;
class scConfigOTA;

class scDespachanteComandos {
public:
    scDespachanteComandos();
    // Injecao de Dependencias: O despachante precisa conversar com a rede e o OTA
    void inicializar(scGestorRede* gestorRede, scConfigOTA* configOTA);
    void processarMensagem(String payloadJson);

private:
    scGestorRede* _gestorRede;
    scConfigOTA* _configOTA;
    
    void resolverComandoUniversal(String comando);
};

#endif // SC_DESPACHANTE_COMANDOS_H
