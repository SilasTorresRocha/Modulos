#ifndef SC_AVISOS_SONOROS_H
#define SC_AVISOS_SONOROS_H

#include <Arduino.h>

enum PadraoSom {
  SILENCIO,
  BIP_CURTO,          // 1 bip rapido (Feedback tatil/menu)
  BIP_LONGO,          // 1 bip demorado (Acao finalizada/salva)
  BIP_DUPLO,          // 2 bips rapidos (Feedback de sucesso/concluido)
  SIRENE_EMERGENCIA,  // Som agressivo continuo para falhas criticas (ex:
                      // hardware travado)
  ALARME_TEMPORIZADOR // Som ritmico infinito (ex: timer do forno, despertador
                      // diário)
};

class scAvisosSonoros {
public:
  scAvisosSonoros(uint8_t pinoBuzzer);

  void inicializar();

  // Comando assincrono para tocar algo.
  // Pode ser chamado em qualquer lugar. Se um som ja estiver tocando, sera
  // interrompido pelo novo.
  void tocar(PadraoSom padrao);

  // Liga ou desliga o modo silencioso de conforto (ATENÇÃO: Não silencia a
  // SIRENE_EMERGENCIA)
  void setMute(bool mutado);
  bool Mutado();

  // O motor da maquina de estados. Deve rodar no loop sem delays.
  void atualizar();

private:
  uint8_t _pino;
  PadraoSom _padraoAtual;
  uint32_t _ultimoTempo;
  uint8_t _passoAtual;
  bool _mutado;

  // Metodo interno para uso de Buzzer Passivo via PWM (tone/noTone)
  void definirPino(bool ligado, unsigned int frequencia = 2000);
};

#endif // SC_AVISOS_SONOROS_H
