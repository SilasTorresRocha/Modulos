#ifndef SC_GESTOR_RELES_H
#define SC_GESTOR_RELES_H

#include <Arduino.h>
#include "../../LibsGlobais/scLogger/scLogger.h"
#include "../../LibsGlobais/scArmazenamentoLocal/scArmazenamentoLocal.h"
#include "../../LibsGlobais/scRelogioSincronizado/scRelogioSincronizado.h"

// Estrutura para manter o contexto de cada relé isolado
struct ContextoRele {
    uint8_t pino;
    bool estadoLigado;         // True = Aparelho energizado
    bool bloqueioGasAtivo;     // Se true, o rele ignora comandos manuais
    
    // Configurações persistentes
    float potenciaW;           // pot_w: Potência do aparelho plugado
    uint8_t regraGas;          // fb_gas: 1 (Ignorar), 2 (Bloquear/Desligar), 3 (Forçar/Ligar)
    uint8_t retornoPwr;        // ret_pwr: 1 (Sempre OFF), 2 (Sempre ON), 3 (Último Estado)
    
    // Matemática de Consumo
    float consumoAcumuladoKWh; // tot_kw: Consumo total
    uint32_t tsUltimoLigamento; // Âncora de tempo (0 se estiver desligado)
    uint32_t tsUltimoSalvo;     // Timestamp da última vez que o KWh foi salvo na flash
};

class scGestorReles {
public:
    scGestorReles();
    
    // Injeta as dependencias e os pinos mapeados
    void inicializar(scLogger* logger, scArmazenamentoLocal* armazenamento, scRelogioSincronizado* relogio, uint8_t pinoR1, uint8_t pinoR2);
    
    // Processamento matemático assíncrono
    void loop();
    
    // Ações de Usuário
    bool setEstadoRele(uint8_t id_rele, bool ligar); // id_rele: 1 ou 2
    bool getEstadoRele(uint8_t id_rele);
    
    // Configurações
    void configurarRele(uint8_t id_rele, float potenciaW, uint8_t regraGas, uint8_t retornoPwr);
    
    // Configurações Isoladas (UI Editor)
    void setPotenciaW(uint8_t id_rele, float potenciaW);
    float getPotenciaW(uint8_t id_rele);
    
    void setRegraGas(uint8_t id_rele, uint8_t regraGas);
    uint8_t getRegraGas(uint8_t id_rele);
    
    void setRetornoPwr(uint8_t id_rele, uint8_t retornoPwr);
    uint8_t getRetornoPwr(uint8_t id_rele);
    
    // Matemática de Energia
    void resetarConsumo(uint8_t id_rele);
    float getConsumoKWh(uint8_t id_rele);
    uint32_t getTempoLigadoSessao(uint8_t id_rele); // Segundos ligados na sessão atual
    
    // Gatilho de Contingência de Risco
    void acionarEmergenciaGas(bool vazamentoDetectado);
    bool EmergenciaGasAtiva(uint8_t id_rele); // Permite à Tela (UI) saber se o botão foi ignorado por motivo de gás

private:
    scLogger* _logger;
    scArmazenamentoLocal* _armazenamento;
    scRelogioSincronizado* _relogio;
    
    ContextoRele _reles[2];
    
    // Mapeamento id_rele (1 ou 2) para o indice do array (0 ou 1)
    inline int _idx(uint8_t id_rele) { return (id_rele == 1) ? 0 : 1; }
    
    void _carregarConfiguracoesDaFlash();
    void _aplicarEstadoFisico(uint8_t id_rele);
    void _salvarEstadoPersistente(uint8_t id_rele);
    void _salvarConsumoPersistente(uint8_t id_rele);
};

#endif // SC_GESTOR_RELES_H
