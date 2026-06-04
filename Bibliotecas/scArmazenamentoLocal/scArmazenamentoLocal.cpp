#include "scArmazenamentoLocal.h"

scArmazenamentoLocal::scArmazenamentoLocal() {
}

void scArmazenamentoLocal::inicializar() {
    // Inicializar LittleFS ou Preferences
}

bool scArmazenamentoLocal::salvarChaveValor(String chave, String valor) {
    // Logica de salvamento com buffer
    return true;
}

String scArmazenamentoLocal::obterValor(String chave) {
    // Logica de obtencao
    return "";
}

bool scArmazenamentoLocal::commitarAlteracoes() {
    // Salvar fisicamente o que estava no buffer
    return true;
}
