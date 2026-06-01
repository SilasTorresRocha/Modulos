#ifndef SC_CONFIG_OTA_H
#define SC_CONFIG_OTA_H

#include <Arduino.h>

class scConfigOTA {
public:
    scConfigOTA();
    
    // Inicia o file system e lê as configurações salvas
    void iniciar();
    
    // Inicia o servidor OTA em background
    void iniciarServidorOTA();
    
    // Deve ser chamada no loop para escutar novos uploads .bin
    void tratarOTA();

    // Salva/Lê credenciais genéricas
    bool salvarCredenciais(const String& chave, const String& valor);
    String lerCredencial(const String& chave);

private:
    bool fileSystemPronto;
};

#endif // SC_CONFIG_OTA_H
