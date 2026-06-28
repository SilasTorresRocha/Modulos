#ifndef SC_GESTOR_BUZZER_M3_H
#define SC_GESTOR_BUZZER_M3_H

#include <Arduino.h>
#include "../../LibsGlobais/scAvisosSonoros/scAvisosSonoros.h" // Biblioteca Global Base

#define PINO_BUZZER 11
#define CANAL_PWM_BUZZER 1

class scLogger;

class scGestorBuzzerM3 {
private:
    scAvisosSonoros _avisos;
    scLogger* _logger;

public:
    // Construtor: Repassa imediatamente o pino para a alocacao estatica da lib global
    scGestorBuzzerM3(uint8_t pinoBuzzer);

    // Injeta a dependencia de Log
    void inicializar(scLogger* logger);

    // ==========================================
    // Regras de Negocio Específicas do Hub (M3)
    // ==========================================
    
    // Tocado a cada evento Touch na tela LVGL
    void tocarBipUI();
    
    // Invocado pelo Radar ou Despachante em caso de Gás ou Queda Geral
    void tocarSireneEmergencia();
    
    // Invocado quando uma configuracao eh salva no SD ou repassada com sucesso
    void tocarSucesso();

    // ==========================================
    // Repasse Direto (Wrapper)
    // ==========================================
    
    // Silencia os bips (ATENCAO: Sirene ignora o mute por seguranca)
    void setMute(bool mutado);
    
    bool isMutado();
    
    // O motor Bare-Metal. Deve rodar no scAgendadorTarefas (Ex: a cada 10ms)
    void atualizar();
};

#endif // SC_GESTOR_BUZZER_M3_H
