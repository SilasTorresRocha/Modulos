#ifndef SC_DESPACHANTE_COMANDOS_H
#define SC_DESPACHANTE_COMANDOS_H

#include <Arduino.h>
#include <ArduinoJson.h>

class scRelogioSincronizado;
class scConfigOTA;
class scLogger;
class scArmazenamentoLocal;
class scTransceptorESPNow;

// Tipo de funcao callback para o Módulo tratar seus proprios comandos (ex: "set_rele")
// Recebe o nome do comando e o objeto JSON de argumentos
typedef void (*CallbackComandoLocal)(const char* cmd, JsonVariant args);

// Tipo de funcao callback para o Módulo escutar a malha passivamente (Mesh)
typedef void (*CallbackPromiscuoLocal)(const char* macOrigem, const char* payload);

class scDespachanteComandos {
public:
    scDespachanteComandos();

    // Injeta as ferramentas que o despachante precisa para operar a placa
    void inicializar(String macDestaPlaca, 
                     scRelogioSincronizado* relogio, 
                     scConfigOTA* ota, 
                     scLogger* logger,
                     scArmazenamentoLocal* armazenamento,
                     scTransceptorESPNow* transceptor);

    // O arquivo .ino do Modulo registra aqui a sua funcao de regras de negocio
    void registrarCallbackLocal(CallbackComandoLocal callback);

    // Registra uma escuta irrestrita (Promíscua) para ler telemetria alheia (Mesh)
    void registrarCallbackPromiscuo(CallbackPromiscuoLocal callback);

    // Onde a scMQTTLib e o ESP-NOW entregam a String recebida para ser dissecada
    void processarPayload(const char* macOrigemTransceptor, const char* payload);

private:
    String _macLocal;
    scRelogioSincronizado* _relogio;
    scConfigOTA* _ota;
    scLogger* _logger;
    scArmazenamentoLocal* _armazenamento;
    scTransceptorESPNow* _transceptor;
    
    CallbackComandoLocal _callbackLocal;
    CallbackPromiscuoLocal _callbackPromiscuo;
};

#endif // SC_DESPACHANTE_COMANDOS_H
