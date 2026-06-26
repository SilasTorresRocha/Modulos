#include "scArmazenamentoLocal.h"

scArmazenamentoLocal::scArmazenamentoLocal() {}

void scArmazenamentoLocal::inicializar() {
#if defined(ESP8266)
  LittleFS.begin();
#elif defined(ESP32)
  _preferencias.begin("app", false); // false = modo Leitura e Escrita
#endif
}

bool scArmazenamentoLocal::salvarChaveValor(String chave, String valor) {
#if defined(ESP8266)
  // No LittleFS, cada chave vira um arquivo .txt
  File f = LittleFS.open("/" + chave + ".txt", "w");
  if (!f)
    return false;
  f.print(valor);
  f.close();
  return true;
#elif defined(ESP32)
  // No ESP32, grava direto no NVS
  _preferencias.putString(chave.c_str(), valor);

  // Como o NVS nao tem um iterador nativo facil de chaves,
  // mantem um "indice mestre" para as chaves de crash
  if (chave.startsWith("crash_")) {
    String indice = _preferencias.getString("_idc_crash", "");
    if (indice.indexOf(chave + ",") == -1) {
      indice += chave + ",";
      _preferencias.putString("_idc_crash", indice);
    }
  }
  return true;
#endif
}

String scArmazenamentoLocal::obterValor(String chave) {
#if defined(ESP8266)
  File f = LittleFS.open("/" + chave + ".txt", "r");
  if (!f)
    return "";
  String conteudo = f.readString();
  f.close();
  return conteudo;
#elif defined(ESP32)
  return _preferencias.getString(chave.c_str(), "");
#endif
}

bool scArmazenamentoLocal::removerChave(String chave) {
#if defined(ESP8266)
  return LittleFS.remove("/" + chave + ".txt");
#elif defined(ESP32)
  bool removido = _preferencias.remove(chave.c_str());

  // Remove do indice mestre tambem
  if (chave.startsWith("crash_")) {
    String indice = _preferencias.getString("_idc_crash", "");
    indice.replace(chave + ",", "");
    _preferencias.putString("_idc_crash", indice);
  }
  return removido;
#endif
}

String scArmazenamentoLocal::listarChavesComPrefixo(String prefixo) {
#if defined(ESP8266)
  String chaves = "";
  Dir dir = LittleFS.openDir("/");
  while (dir.next()) {
    String nome = dir.fileName(); // ex: "crash_wifi.txt"
    if (nome.startsWith(prefixo)) {
      nome.replace(".txt", ""); // limpa extensao
      chaves += nome + ",";
    }
  }
  return chaves;
#elif defined(ESP32)
  // Usa indice mestre para resolver o prefixo de crash
  if (prefixo == "crash_") {
    return _preferencias.getString("_idc_crash", "");
  }
  return "";
#endif
}

bool scArmazenamentoLocal::commitarAlteracoes() {
  // Tanto LittleFS (ao fechar o arquivo) quanto Preferences gravam instantaneamente na flash
  // Mas vou manter isso na interface para se no futuro precisar usar um chip que exige flush manual, ja estar aqui
  return true;
}
