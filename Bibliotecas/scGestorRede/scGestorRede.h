#ifndef SC_GESTOR_REDE_H
#define SC_GESTOR_REDE_H

#include <Arduino.h>

class scGestorRede {
public:
    scGestorRede();
    
    // Inicia a gestão de rede (tenta conectar ao Wi-Fi primário)
    void iniciar(const char* ssid, const char* password);
    
    // Deve ser chamada no loop() principal para lidar com o chaveamento
    void atualizar();
    
    // Retorna se o dispositivo está no Wi-Fi normal ou operando via ESP-NOW
    bool estaEmFallback();

private:
    void ativarESPNow();
    void escanearCanaisESPNow();
    void tentarReconectarWiFi();
    
    unsigned long ultimoPingRetorno;
    bool modoFallbackAtivo;
};

#endif // SC_GESTOR_REDE_H
