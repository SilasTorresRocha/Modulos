#ifndef SC_SAUDE_HARDWARE_H
#define SC_SAUDE_HARDWARE_H

#include <Arduino.h>

#if defined(ESP32)
  #include <esp_task_wdt.h>
#endif

class scArmazenamentoLocal;

class scSaudeHardware {
public:
    scSaudeHardware();
    
    // Inicia o Watchdog Timer e resgata uptime anterior se existir
    void inicializar(uint32_t timeoutWatchdogSegundos = 5, scArmazenamentoLocal* armazenamento = nullptr);
    
    // Deve ser chamado a cada iteracao do loop() para avisar o HW que estamos vivos
    void alimentarWatchdog();
    
    // Coleta a RAM livre para prevencao de vazamentos (Memory Leaks)
    uint32_t getRamLivre();
    
    // Uptime da Sessao Atual (zera se a placa reiniciar)
    uint32_t getUptimeSessaoSegundos();
    
    // Uptime Estavel (mantido se o reiniciar for por prevencao)
    uint32_t getUptimeEstavelSegundos();
    
    // Deve ser chamado no loop(). Se for 03:00, salva uptime e reinicia.
    void verificarRebootPreventivo(uint8_t horaAtual, uint8_t minutoAtual);
    
    // Retorna a temperatura do chip se suportado, senao 0.0
    float getTemperaturaInterna();

private:
    uint32_t _uptimeInicial;
    uint32_t _uptimeAcumulado;
    scArmazenamentoLocal* _armazenamento;
};

#endif // SC_SAUDE_HARDWARE_H
