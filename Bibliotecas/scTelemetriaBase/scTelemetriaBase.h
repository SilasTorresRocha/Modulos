#ifndef SC_TELEMETRIA_BASE_H
#define SC_TELEMETRIA_BASE_H

#include <Arduino.h>
#include <vector>

class scMQTTLib;
class scLogger;
class scSaudeHardware;

class scTelemetriaBase {
public:
    scTelemetriaBase();
    
    // Injeta as dependencias e prepara o buffer anti-fragmentacao
    void inicializar(scMQTTLib* mqtt, scLogger* logger, scSaudeHardware* saude, String macOrigem, String tipoModulo);
    
    // Inicia um novo pacote limpando o anterior e preenchendo as variaveis base
    void iniciarPacote();
    
    // Métodos para o codigo principal injetar suas variaveis 
    void adicionarInteiro(const String& chave, int valor);
    void adicionarFloat(const String& chave, float valor, int casasDecimais = 2);
    void adicionarBool(const String& chave, bool valor);
    
    // ATENCAO: Nao enviar strings contendo aspas duplas ("), pois o parser manual quebra. Use apenas texto limpo.
    void adicionarString(const String& chave, const String& valor);
    
    // Adiciona um codigo de erro ao array "err"
    void adicionarErro(const String& erroCodigo);
    
    // Fecha o JSON, verifica o limite critico do ESP-NOW (250 bytes) e despacha
    void despachar();

private:
    scMQTTLib* _mqtt;
    scLogger* _logger;
    scSaudeHardware* _saude;
    String _macOrigem;
    String _tipoModulo;
    
    String _jsonAtual;
    std::vector<String> _erros;
};

#endif // SC_TELEMETRIA_BASE_H
