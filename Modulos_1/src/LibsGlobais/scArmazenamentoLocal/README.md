# scArmazenamentoLocal

## Responsabilidade no Ecossistema
A biblioteca `scArmazenamentoLocal` atua como o Gerenciador de Estado da aplicação, lendo e gravando na memória Flash de forma segura.

## Papel no Projeto
- Fornecer uma abstração sobre o LittleFS ou Preferences (NVS) para gravar o "estado" da placa (ex: agendamentos salvos fisicamente, último status dos relés antes da queda de energia).
- Proteger a Flash contra escrita excessiva (*Wear Leveling* lógico): ela só realiza a gravação física (commit) se houve alteração nos dados em relação ao que está na RAM ou salva em janelas periódicas de tempo (batch).
- Permitir que o Módulo 2 e 3 mantenham a "Tolerância a Falhas" operando os seus hardwares via consultas rápidas à RAM ou persistindo configs locais sem perder dados no reboot.
