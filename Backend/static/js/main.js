// Estado Global
let isPolling = false;
let pollingInterval;
let ultimosDados = [];

// MACs dinâmicos descobertos via telemetria
let macM1 = null;
let macM2 = null;

// Ao carregar a página
document.addEventListener("DOMContentLoaded", () => {
    iniciarPolling();
    atualizarRelogio();
    setInterval(atualizarRelogio, 1000);
});

function atualizarRelogio() {
    const agora = new Date();
    document.getElementById("last-update").textContent = agora.toLocaleTimeString('pt-BR');
}

function iniciarPolling() {
    if (isPolling) return;
    isPolling = true;
    
    // Busca imediatamente a primeira vez
    buscarTelemetria();
    
    // Repete a cada 2 segundos
    pollingInterval = setInterval(buscarTelemetria, 2000);
}

function forcarAtualizacao() {
    // Adiciona efeito visual no botão
    const btn = document.querySelector(".btn-primary i.fa-rotate-right");
    btn.classList.add("fa-spin");
    
    buscarTelemetria().then(() => {
        setTimeout(() => btn.classList.remove("fa-spin"), 500);
    });
}

async function buscarTelemetria() {
    try {
        // Chama o proxy seguro no FastAPI (que não expõe a chave)
        const response = await fetch('/api/telemetria?limite=50');
        
        if (!response.ok) {
            throw new Error(`Erro HTTP: ${response.status}`);
        }
        
        const dados = await response.json();
        ultimosDados = dados;
        
        processarDados(dados);
        desenharTerminal(dados);
        
        // Indicador de API OK
        document.getElementById("mqtt-pulse").className = "pulse-dot green";
        document.getElementById("mqtt-text").textContent = "Sincronizado";
        
    } catch (error) {
        console.error("Falha ao buscar telemetria:", error);
        document.getElementById("mqtt-pulse").className = "pulse-dot red";
        document.getElementById("mqtt-text").textContent = "Desconectado";
    }
}

function processarDados(historico) {
    // Como a API retorna uma lista ordenada (do mais recente para o mais antigo),
    // vamos pegar apenas a mensagem MAIS RECENTE de cada módulo para desenhar nos cards.
    
    let m1Atualizado = false;
    let m2Atualizado = false;

    for (const msg of historico) {
        // Já atualizamos os dois cards?
        if (m1Atualizado && m2Atualizado) break;
        
        if (!msg.payload || !msg.payload.tipo) continue;

        const tipo = msg.payload.tipo;
        const dados = msg.payload.dados;

        if (tipo === "M1" && !m1Atualizado) {
            macM1 = msg.payload.mac_origem;
            atualizarCardM1(macM1, dados, msg.timestamp);
            m1Atualizado = true;
        } else if (tipo === "M2" && !m2Atualizado) {
            macM2 = msg.payload.mac_origem;
            atualizarCardM2(macM2, dados, msg.timestamp);
            m2Atualizado = true;
        }
    }
}

function atualizarCardM1(mac, dados, timestamp) {
    document.getElementById("m1-mac").textContent = mac;
    
    document.getElementById("m1-temp").textContent = dados.temp ? dados.temp.toFixed(1) : "--";
    document.getElementById("m1-gas").textContent = dados.gas !== undefined ? dados.gas : "--";
    document.getElementById("m1-forno").textContent = dados.t_forno !== undefined ? dados.t_forno : "--";
    document.getElementById("m1-rssi").textContent = dados.rssi !== undefined ? dados.rssi : "--";
    
    // Alerta Visual de Gás (Estético)
    const boxGas = document.getElementById("gas-box");
    const iconGas = document.getElementById("gas-icon");
    if (dados.gas > 600) { // Valor hipotético de alerta para UI
        boxGas.classList.add("alert");
        iconGas.className = "fa-solid fa-cloud-showers-heavy icon-red";
    } else {
        boxGas.classList.remove("alert");
        iconGas.className = "fa-solid fa-cloud icon-green";
    }
}

function atualizarCardM2(mac, dados, timestamp) {
    document.getElementById("m2-mac").textContent = mac;
    
    if (dados.r1_st !== undefined) {
        document.getElementById("toggle-r1").checked = dados.r1_st;
    }
    if (dados.r2_st !== undefined) {
        document.getElementById("toggle-r2").checked = dados.r2_st;
    }
    
    document.getElementById("m2-r1kw").textContent = dados.r1_kw !== undefined ? dados.r1_kw.toFixed(3) : "--";
    document.getElementById("m2-r2kw").textContent = dados.r2_kw !== undefined ? dados.r2_kw.toFixed(3) : "--";
    document.getElementById("m2-totkw").textContent = dados.tot_kw !== undefined ? dados.tot_kw.toFixed(3) : "--";
}

function desenharTerminal(historico) {
    const term = document.getElementById("log-window");
    term.innerHTML = ""; // Limpa
    
    // Pega os 10 mais recentes para não poluir
    const limitados = historico.slice(0, 15);
    
    limitados.forEach(msg => {
        const timeStr = new Date(msg.timestamp).toLocaleTimeString('pt-BR');
        const div = document.createElement("div");
        div.className = "log-entry";
        
        let payloadStr = "";
        if (typeof msg.payload === 'object') {
            payloadStr = JSON.stringify(msg.payload);
        } else {
            payloadStr = msg.payload || "{}";
        }
        
        div.innerHTML = `
            <span class="log-time">[${timeStr}]</span> 
            <span class="log-topic">${msg.topico}</span> 
            <span class="log-payload">${payloadStr}</span>
        `;
        term.appendChild(div);
    });
}

// ==========================================
// Ações e Comandos (POST)
// ==========================================

async function dispararComandoBackend(jsonString) {
    try {
        const formData = new FormData();
        formData.append("comando", jsonString);
        
        const response = await fetch('/api/comando', {
            method: 'POST',
            body: formData
        });
        
        if (!response.ok) throw new Error("Falha na API");
        return await response.json();
    } catch (error) {
        console.error(error);
        throw error;
    }
}

// Chamado ao clicar nos botões de Relé (Toggle Switch)
function enviarComandoRele(idRelay, estado) {
    // Desativamos temporariamente para não voltar sozinho por causa do polling
    const toggle = document.getElementById(`toggle-r${idRelay}`);
    toggle.disabled = true;
    
    const payload = {
        "mac_origem": "HUB",
        "mac_destino": "ALL", // Ou MAC_M2, mas ALL funciona se só tiver 1 M2
        "cmd": "set_rele",
        "args": {
            "id": idRelay,
            "estado": estado
        }
    };
    
    dispararComandoBackend(JSON.stringify(payload)).then(() => {
        // Reativa
        toggle.disabled = false;
        // Pede atualização imediata para ver o resultado
        setTimeout(forcarAtualizacao, 500);
    }).catch(() => {
        // Se falhou, volta o switch
        toggle.checked = !estado;
        toggle.disabled = false;
        alert("Erro ao enviar comando para o Relé!");
    });
}

// ==========================================
// Modal de Terminal Raw
// ==========================================
function getTemplates() {
    const destinoM1 = macM1 || "E8:9F:6D:93:3F:34";
    return {
        m1_solicitar: {
            "mac_origem": "HUB",
            "mac_destino": destinoM1,
            "cmd": "solicitar_status",
            "args": {}
        },
        m1_gas: {
            "mac_origem": "HUB",
            "mac_destino": destinoM1,
            "cmd": "configurar_limiar_gas",
            "args": { "limiar": 800 }
        },
        m1_timer: {
            "mac_origem": "HUB",
            "mac_destino": destinoM1,
            "cmd": "configurar_operacao",
            "args": { "alm_prep": 10 }
        }
    };
}

function abrirModalComandos() {
    document.getElementById('modal-comando').style.display = 'block';
    document.getElementById('comando-feedback').textContent = '';
}

function fecharModalComandos() {
    document.getElementById('modal-comando').style.display = 'none';
}

function carregarTemplate() {
    const select = document.getElementById("template-select").value;
    const textarea = document.getElementById("json-payload");
    
    const templates = getTemplates();
    
    if (select && templates[select]) {
        textarea.value = JSON.stringify(templates[select], null, 2);
    } else {
        textarea.value = "";
    }
}

function dispararComando() {
    const raw = document.getElementById("json-payload").value;
    const feedback = document.getElementById("comando-feedback");
    
    if (!raw.trim()) {
        feedback.textContent = "O payload não pode estar vazio!";
        feedback.className = "feedback-text text-error";
        return;
    }
    
    // Validar se é JSON válido antes de enviar
    try {
        JSON.parse(raw);
    } catch (e) {
        feedback.textContent = "Erro: JSON inválido!";
        feedback.className = "feedback-text text-error";
        return;
    }
    
    feedback.textContent = "Enviando...";
    feedback.className = "feedback-text";
    
    dispararComandoBackend(raw).then(res => {
        feedback.textContent = "Enviado com sucesso para a Nuvem!";
        feedback.className = "feedback-text text-success";
        setTimeout(forcarAtualizacao, 1000);
    }).catch(err => {
        feedback.textContent = "Falha ao enviar!";
        feedback.className = "feedback-text text-error";
    });
}

// Fechar modais clicando fora
window.onclick = function(event) {
    const modalCmd = document.getElementById('modal-comando');
    const modalCfg = document.getElementById('modal-config');
    if (event.target == modalCmd) {
        modalCmd.style.display = "none";
    }
    if (event.target == modalCfg) {
        modalCfg.style.display = "none";
    }
}

// ==========================================
// Modal de Configurações Globais
// ==========================================

function abrirModalConfig() {
    document.getElementById('modal-config').style.display = 'block';
    document.getElementById('cfg-feedback').textContent = '';
}

function fecharModalConfig() {
    document.getElementById('modal-config').style.display = 'none';
}

function exibirFeedbackCfg(msg, isError=false) {
    const fb = document.getElementById('cfg-feedback');
    fb.textContent = msg;
    fb.className = isError ? "mono-text feedback-msg text-error" : "mono-text feedback-msg text-success";
    if (!isError) setTimeout(() => fb.textContent = '', 3000);
}

function enviarCfgBackend(payload) {
    dispararComandoBackend(JSON.stringify(payload)).then(() => {
        exibirFeedbackCfg("Comando enviado com sucesso!");
    }).catch(e => {
        exibirFeedbackCfg("Erro ao enviar comando.", true);
    });
}

function enviarCfgM1Tela() {
    const val = parseInt(document.getElementById('cfg-m1-tela').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM1 || "ALL",
        cmd: "tela_idle",
        args: { tela: val }
    });
}

function enviarCfgM1Temp() {
    const val = parseFloat(document.getElementById('cfg-m1-temp').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM1 || "ALL",
        cmd: "configurar_operacao",
        args: { temp_abs: val }
    });
}

function enviarCfgM1Timer() {
    const val = parseInt(document.getElementById('cfg-m1-prep').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM1 || "ALL",
        cmd: "configurar_operacao",
        args: { alm_prep: val }
    });
}

function enviarCfgM1Crit() {
    const val = parseInt(document.getElementById('cfg-m1-crit').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM1 || "ALL",
        cmd: "configurar_operacao",
        args: { alm_crit: val }
    });
}

function enviarCfgM2Gas() {
    const val = parseInt(document.getElementById('cfg-m2-gas').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM2 || "ALL",
        cmd: "configurar_limiar_gas",
        args: { limiar: val }
    });
}

function enviarCfgM2Tela() {
    const val = parseInt(document.getElementById('cfg-m2-tela').value);
    enviarCfgBackend({
        mac_origem: "HUB",
        mac_destino: macM2 || "ALL",
        cmd: "tela_idle",
        args: { tela: val }
    });
}

function enviarCfgM2Reset(idRele) {
    if (confirm(`Tem certeza que deseja zerar o consumo do Canal ${idRele}?`)) {
        enviarCfgBackend({
            mac_origem: "HUB",
            mac_destino: macM2 || "ALL",
            cmd: "reset_kwh",
            args: { id: idRele }
        });
    }
}
