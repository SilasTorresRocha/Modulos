/**
 * @file Modulos_3_V2.ino
 * @brief Firmware Central (Hub M3) - Arquitetura Bare-Metal Industrial
 * 
 * Este arquivo é estritamente limpo por design. Toda a lógica de injeção 
 * de dependências, controle de hardware, Roteamento ESP-NOW/MQTT e 
 * renderização gráfica (LVGL) foi isolada dentro do Maestro (scAgendadorTarefasM3).
 */

#include <Arduino.h>
#include "src/Libs/scAgendadorTarefasM3/scAgendadorTarefasM3.h"

// O Container Estático de Injeção de Dependências e Orquestrador Central
scAgendadorTarefasM3 hubCentral;

void setup() {
    // Inicializa fisicamente todos os 12 subsistemas na ordem correta de boot
    hubCentral.inicializar();
}

void loop() {
    // Pipeline Sequencial Estrito. Roda milhares de vezes por segundo, ditando
    // a sobrevida e estabilidade extrema do ESP32-S2:
    // 
    // 1. Drena as ISRs de Rede (ESP-NOW/MQTT)
    // 2. Escaneia a malha por quedas de sinal (Radar Dead Man's Switch)
    // 3. Renderiza os blocos da Tela TFT (Motor Grafico LVGL)
    // 4. Empacota JSON estático e Salva Log Idempotente no Cartao SD
    // 5. Mede a saude da CPU e aciona Hard-Reset caso trave
    hubCentral.processar();
}
