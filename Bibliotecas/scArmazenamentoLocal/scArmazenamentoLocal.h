#ifndef SC_ARMAZENAMENTO_LOCAL_H
#define SC_ARMAZENAMENTO_LOCAL_H

#include <Arduino.h>

// A magica do pre-processador: O compilador so injeta a lib certa para a placa selecionada
#if defined(ESP8266)
  #include <LittleFS.h>
#elif defined(ESP32)
  #include <Preferences.h>
#endif

class scArmazenamentoLocal {
public:
    scArmazenamentoLocal();
    
    // Inicia o file system ou NVS
    void inicializar();
    
    // Salva ou sobrescreve uma String
    bool salvarChaveValor(String chave, String valor);
    
    // Resgata o valor. Retorna String vazia se nao existir
    String obterValor(String chave);
    
    // Apaga um registro fisico
    bool removerChave(String chave);
    
    // Retorna as chaves encontradas separadas por virgula (ex: "crash_rede,crash_app,")
    String listarChavesComPrefixo(String prefixo); 
    
    // Força gravação em flash (se a plataforma exigir buffer)
    bool commitarAlteracoes();

private:
#if defined(ESP32)
    Preferences _preferencias; // Objeto exclusivo do Módulo 3 (Hub) e outros ESP32
#endif
};

#endif // SC_ARMAZENAMENTO_LOCAL_H
