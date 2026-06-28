#ifndef SC_RADAR_ECOSSISTEMA_M3_H
#define SC_RADAR_ECOSSISTEMA_M3_H

#include <Arduino.h>
#include <string.h>

class scLogger;
class scGestorBuzzerM3;

#define MAX_NODOS_RADAR 20
#define TAM_MAC_RADAR 18

enum EstadoNo {
    VERDE_ONLINE,
    AMARELO_FALLBACK,
    VERMELHO_MORTO
};

struct InfoNo {
    char mac[TAM_MAC_RADAR];
    char tipo[4]; // Guarda o tipo do contrato (Ex: "M1\0", "M4\0")
    uint32_t ultimoPingMillis;
    bool viaEspNow;
    EstadoNo estadoAtual;
    bool ocupado;
};

class scRadarEcossistemaM3 {
private:
    scLogger* _logger;
    scGestorBuzzerM3* _buzzer;
    InfoNo _nodos[MAX_NODOS_RADAR];
    bool _emergenciaGeralAtiva;
    uint32_t _ultimoAlarmeTratado;

    // Métodos utilitários de Busca na Memória Estática
    int _encontrarIndice(const char* mac);
    int _encontrarSlotVazio();

public:
    scRadarEcossistemaM3();

    // Inicialização do Subsistema com suas injeções
    void inicializar(scLogger* logger, scGestorBuzzerM3* buzzer);

    // O Roteador/Bridge deve bater neste método sempre que ouvir um MAC na rede (MQTT ou ESP-NOW)
    void registrarPing(const char* mac, bool viaEspNow, const char* tipoModulo = "UKN");

    // Função de varredura mecânica do Dead Man's Switch. Roda não-bloqueante no Super Loop.
    void processar();

    // Retorna Flag indicando se a casa tem algum Ponto Cego (Falha de M1)
    bool isEmergenciaGeral() const;

    // Consulta à Tabela para a UI
    bool existeModuloDoTipo(const char* tipo) const;

    // Retorna a cor da saúde de um Node Específico
    EstadoNo obterEstado(const char* mac);
};

#endif // SC_RADAR_ECOSSISTEMA_M3_H
