#include "scGestorAgendamentos.h"

scGestorAgendamentos::scGestorAgendamentos() {
  _logger = nullptr;
  _relogio = nullptr;
  _armazenamento = nullptr;
  _reles = nullptr;
  _ultimoMinutoChecado = -1;

  for (int i = 0; i < 10; i++) {
    _agendamentos[i].ativo = false;
    _agendamentos[i].executado_hoje = false;
  }
}

void scGestorAgendamentos::inicializar(scLogger *logger,scRelogioSincronizado *relogio,scArmazenamentoLocal *armazenamento,scGestorReles *reles) {
  _logger = logger;
  _relogio = relogio;
  _armazenamento = armazenamento;
  _reles = reles;

  if (!_logger)
    return; // Regra 9
  if (!_relogio || !_armazenamento || !_reles) {
    _logger->erro("GESTOR_AGENDAMENTOS", "Ponteiros injetados nulos. Motor cronologico desativado.");
    return;
  }

  _carregarDaMemoria();
}

uint8_t scGestorAgendamentos::_converterDiaParaInt(String diaStr) {
  diaStr.toUpperCase();
  if (diaStr == "DOM")
    return 0;
  if (diaStr == "SEG")
    return 1;
  if (diaStr == "TER")
    return 2;
  if (diaStr == "QUA")
    return 3;
  if (diaStr == "QUI")
    return 4;
  if (diaStr == "SEX")
    return 5;
  if (diaStr == "SAB")
    return 6;
  if (diaStr == "TODOS")
    return 8; // O Curinga
  return 8;   // Default caso string invalida para garantir execucao
}

void scGestorAgendamentos::_carregarDaMemoria() {
  if (!_armazenamento)
    return;

  String raw = _armazenamento->obterValor("agd_mem");
  if (raw == "")
    return; // Sem historico

  // Parse da String comprimida: 1:1:8:18:30:1:1|2:1:0:...
  int inicioIndex = 0;
  int arrayIndex = 0;

  while (inicioIndex < raw.length() && arrayIndex < 10) {
    int pipeIndex = raw.indexOf('|', inicioIndex);
    if (pipeIndex == -1)
      pipeIndex = raw.length();

    String slotInf = raw.substring(inicioIndex, pipeIndex);

    int d1 = slotInf.indexOf(':');
    int d2 = slotInf.indexOf(':', d1 + 1);
    int d3 = slotInf.indexOf(':', d2 + 1);
    int d4 = slotInf.indexOf(':', d3 + 1);
    int d5 = slotInf.indexOf(':', d4 + 1);
    int d6 = slotInf.indexOf(':', d5 + 1);

    if (d1 != -1 && d6 != -1) {
      _agendamentos[arrayIndex].id_agd = slotInf.substring(0, d1).toInt();
      _agendamentos[arrayIndex].rele = slotInf.substring(d1 + 1, d2).toInt();
      _agendamentos[arrayIndex].diaSemana =
          slotInf.substring(d2 + 1, d3).toInt();
      _agendamentos[arrayIndex].hora = slotInf.substring(d3 + 1, d4).toInt();
      _agendamentos[arrayIndex].min = slotInf.substring(d4 + 1, d5).toInt();
      _agendamentos[arrayIndex].acao_ligar =
          slotInf.substring(d5 + 1, d6).toInt() == 1;
      _agendamentos[arrayIndex].ativo = slotInf.substring(d6 + 1).toInt() == 1;
      _agendamentos[arrayIndex].executado_hoje = false; // Armado para uso
    }

    inicioIndex = pipeIndex + 1;
    arrayIndex++;
  }
}

void scGestorAgendamentos::_salvarNaMemoria() {
  if (!_armazenamento)
    return;

  String finalData = "";
  for (int i = 0; i < 10; i++) {
    if (_agendamentos[i].ativo) {
      finalData += String(_agendamentos[i].id_agd) + ":" +
                   String(_agendamentos[i].rele) + ":" +
                   String(_agendamentos[i].diaSemana) + ":" +
                   String(_agendamentos[i].hora) + ":" +
                   String(_agendamentos[i].min) + ":" +
                   String(_agendamentos[i].acao_ligar ? "1" : "0") + ":1|";
    }
  }

  // Remove o ultimo separador | para manter a string limpa
  if (finalData.endsWith("|")) {
    finalData.remove(finalData.length() - 1);
  }

  _armazenamento->salvarChaveValor("agd_mem", finalData);
}

bool scGestorAgendamentos::adicionarAgendamento(uint8_t id, uint8_t rele,
String diaStr, uint8_t hora,uint8_t min, bool ligar) {
  if (id < 1 || id > 10)
    return false;

  int idx = id - 1; // Array base 0
  _agendamentos[idx].id_agd = id;
  _agendamentos[idx].rele = rele;
  _agendamentos[idx].diaSemana = _converterDiaParaInt(diaStr);
  _agendamentos[idx].hora = hora;
  _agendamentos[idx].min = min;
  _agendamentos[idx].acao_ligar = ligar;
  _agendamentos[idx].ativo = true;
  _agendamentos[idx].executado_hoje =
      false; // Ja prepara para atirar hoje se a hora estiver chegando

  _salvarNaMemoria();
  if (_logger)
    _logger->info("GESTOR_AGENDAMENTOS", "Agendamento " + String(id) +" registrado no slot " +String(idx) + ".");
  return true;
}

bool scGestorAgendamentos::excluirAgendamento(uint8_t id) {
  if (id < 1 || id > 10)
    return false;

  int idx = id - 1;
  if (_agendamentos[idx].ativo) {
    _agendamentos[idx].ativo = false;
    _salvarNaMemoria();
    if (_logger)
      _logger->info("GESTOR_AGENDAMENTOS","Agendamento " + String(id) +" cancelado/apagado com sucesso.");
    return true;
  }
  return false;
}

Agendamento *scGestorAgendamentos::listarAgendamentos() {
  return _agendamentos;
}

void scGestorAgendamentos::loop() {
  if (!_relogio || !_reles)
    return;
  if (!_relogio->estaSincronizado())
    return; // Cego (1970). Bloqueia tudo para nao disparar alarmes indevidos no
            // boot offline.

  uint32_t unixTime = _relogio->obterHoraUnix();
  int minAt = (unixTime % 3600) / 60;

  // Otimizacao: so realiza matematica completa no exato momento que o relogio
  // vira de minuto
  if (minAt == _ultimoMinutoChecado)
    return;
  _ultimoMinutoChecado = minAt;

  int horaAt = (unixTime % 86400) / 3600;
  int diaSemanaAt = ((unixTime / 86400) + 4) % 7; // Unix comeca na Quinta (4)

  for (int i = 0; i < 10; i++) {
    if (!_agendamentos[i].ativo)
      continue;

    // Se ja passou do minuto de alarme do slot, desarma o bloqueio
    // "executado_hoje" para que a regra funcione novamente no proximo
    // dia/semana da vida
    if (_agendamentos[i].hora != horaAt || _agendamentos[i].min != minAt) {
      _agendamentos[i].executado_hoje = false;
    }

    // Verifica combinacao
    bool matchDia = (_agendamentos[i].diaSemana == 8) || (_agendamentos[i].diaSemana == diaSemanaAt);
    bool matchHora = (_agendamentos[i].hora == horaAt) && (_agendamentos[i].min == minAt);

    if (matchDia && matchHora && !_agendamentos[i].executado_hoje) {
      // A hora chegou!
      _agendamentos[i].executado_hoje = true; // Impede que chame digitalWrite novamente nos proximos 60 segundos

      bool ok = _reles->setEstadoRele(_agendamentos[i].rele,_agendamentos[i].acao_ligar);

      if (_logger) {
        if (ok) {
          _logger->info("GESTOR_AGENDAMENTOS","GATILHO: Disparo Autonomo do Agendamento [" +String(_agendamentos[i].id_agd) +"] realizado com sucesso.");
        } else {
          _logger->erro("GESTOR_AGENDAMENTOS","FALHA: Agendamento [" +String(_agendamentos[i].id_agd) + "] interceptado pela Segurança de Gas!");
        }
      }
    }
  }
}
