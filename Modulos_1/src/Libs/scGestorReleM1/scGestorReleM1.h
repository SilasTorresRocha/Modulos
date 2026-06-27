#ifndef SC_GESTOR_RELE_M1_H
#define SC_GESTOR_RELE_M1_H

#include "../../LibsGlobais/scLogger/scLogger.h"
#include <Arduino.h>

class scGestorReleM1 {
public:
  scGestorReleM1();

  // Inicializa a classe com o logger mandatório e o pino físico
  // logicaInvertida: true para módulos de relé normais (LOW aciona), false para
  // a maioria dos SSRs (HIGH aciona).
  void inicializar(scLogger *logger, uint8_t pinoRele,
                   bool logicaInvertida = false);

  // Método de atuação principal (geralmente invocado pelo callback do
  // scMonitorGasM1 na main)
  void acionarEmergencia(bool ligarCarga);

  // Getter essencial para inclusão no empacotamento da Telemetria
  bool CargaLigada() const;

private:
  scLogger *_logger;
  uint8_t _pinoRele;
  bool _logicaInvertida;
  bool _cargaLigada;

  // Atualiza o estado físico do pino mantendo o isolamento lógico
  void atualizarHardware();
};

#endif // SC_GESTOR_RELE_M1_H
