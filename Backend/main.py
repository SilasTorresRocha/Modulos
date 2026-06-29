import asyncio
import os
import httpx
import json
import logging
from fastapi import FastAPI, Request, Form, HTTPException
from fastapi.responses import HTMLResponse
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel
import uvicorn

logging.basicConfig(level=logging.INFO)
logger = logging.getLogger("RouterHUB")

app = FastAPI(title="Dashboard Ecossistema IoT")

# Configuração de templates e arquivos estáticos
templates = Jinja2Templates(directory="templates")
app.mount("/static", StaticFiles(directory="static"), name="static")

# Chave fixa fornecida
API_KEY = os.getenv("API_KEY", "iUCrkaBILIhhAI_2qB1gVlUgkpo-pbXtAcYB9n7xv3o")
BASE_URL = "https://apisilas.ddns.net/api/v1"

class ComandoPayload(BaseModel):
    comando: str

# Estado do roteador background
ultima_temp_roteada = None
ultimo_gas_roteado = None

async def hub_router_task():
    """
    Tarefa de background (24/7) que faz o papel do HUB físico.
    Busca telemetria a cada 60 segundos e repassa pro M2 se M1 atualizou.
    """
    global ultima_temp_roteada, ultimo_gas_roteado
    logger.info("Iniciando HUB Virtual no Backend (Poller 60s)...")
    
    async with httpx.AsyncClient() as client:
        while True:
            try:
                # Busca apenas as últimas mensagens para não pesar
                url_get = f"{BASE_URL}/telemetria?chave_api={API_KEY}&limite=5"
                resp = await client.get(url_get, timeout=10.0)
                resp.raise_for_status()
                dados = resp.json()
                
                # Procura a mensagem mais recente do M1
                for item in dados:
                    if 'payload' in item and isinstance(item['payload'], str):
                        try:
                            payload_json = json.loads(item['payload'])
                            if payload_json.get("tipo") == "M1" and "dados" in payload_json:
                                dados_m1 = payload_json["dados"]
                                temp_m1 = dados_m1.get("temp")
                                gas_m1 = dados_m1.get("gas")
                                
                                # Verifica se teve alguma mudança
                                if temp_m1 != ultima_temp_roteada or gas_m1 != ultimo_gas_roteado or (dados_m1.get("t_forno", 0) >= dados_m1.get("alm_crit", 999999) and dados_m1.get("alm_crit", 0) > 0):
                                    ultima_temp_roteada = temp_m1
                                    ultimo_gas_roteado = gas_m1
                                    
                                    # Monta o comando de roteamento
                                    cmd_router = {
                                        "mac_origem": "HUB",
                                        "mac_destino": "ALL",
                                        "cmd": "repassar_telemetria",
                                        "args": {
                                            "tipo": "M1"
                                        }
                                    }
                                    if temp_m1 is not None: cmd_router["args"]["temp"] = temp_m1
                                    if gas_m1 is not None: cmd_router["args"]["gas"] = gas_m1
                                    if "t_forno" in dados_m1: cmd_router["args"]["t_forno"] = dados_m1["t_forno"]
                                    if "alm_crit" in dados_m1: cmd_router["args"]["alm_crit"] = dados_m1["alm_crit"]
                                    
                                    # Dispara o POST
                                    url_post = f"{BASE_URL}/comandos/enviar_externo?chave_api={API_KEY}"
                                    data_post = {"comando": json.dumps(cmd_router)}
                                    await client.post(url_post, data=data_post, timeout=10.0)
                                    logger.info(f"Roteado M1 -> M2: Temp {temp_m1}, Gas {gas_m1}")
                                
                                # Processou o mais recente do M1, interrompe o for
                                break
                        except Exception as e:
                            logger.error(f"Erro ao parsear M1 no background: {e}")
                            
            except Exception as e:
                logger.error(f"Erro no loop do HUB Virtual: {e}")
                
            # Dorme por 60 segundos
            await asyncio.sleep(60)

@app.on_event("startup")
async def startup_event():
    # Inicia a tarefa em segundo plano
    asyncio.create_task(hub_router_task())

@app.get("/", response_class=HTMLResponse)
async def read_root(request: Request):
    """
    Renderiza a página principal do Dashboard.
    """
    return templates.TemplateResponse("index.html", {"request": request})

@app.get("/api/telemetria")
async def get_telemetria(limite: int = 50):
    """
    Proxy seguro para buscar a telemetria na API externa escondendo a Chave de API.
    """
    async with httpx.AsyncClient() as client:
        try:
            url = f"{BASE_URL}/telemetria?chave_api={API_KEY}&limite={limite}"
            response = await client.get(url, timeout=10.0)
            response.raise_for_status()
            
            # A API retorna uma lista de dicionários. Vamos extrair o 'payload' de cada item e converter de string JSON para objeto
            dados = response.json()
            
            # Tratamento inteligente: A API retorna o campo payload como String, vamos parsear para JSON Real
            for item in dados:
                if 'payload' in item and isinstance(item['payload'], str):
                    try:
                        item['payload'] = json.loads(item['payload'])
                    except json.JSONDecodeError:
                        pass
                        
            return dados
        except httpx.RequestError as e:
            raise HTTPException(status_code=502, detail=f"Erro ao conectar com a API principal: {e}")
        except httpx.HTTPStatusError as e:
            raise HTTPException(status_code=e.response.status_code, detail=f"Erro retornado pela API: {e}")

@app.post("/api/comando")
async def enviar_comando(comando: str = Form(...)):
    """
    Proxy seguro para enviar um comando via form data (URL Encoded).
    """
    async with httpx.AsyncClient() as client:
        try:
            url = f"{BASE_URL}/comandos/enviar_externo?chave_api={API_KEY}"
            # O curl -d "comando=..." envia como form data (application/x-www-form-urlencoded)
            data = {"comando": comando}
            
            response = await client.post(url, data=data, timeout=10.0)
            response.raise_for_status()
            return response.json()
        except httpx.RequestError as e:
            raise HTTPException(status_code=502, detail=f"Erro de rede: {e}")
        except httpx.HTTPStatusError as e:
            raise HTTPException(status_code=e.response.status_code, detail=f"API Rejeitou: {e}")

if __name__ == "__main__":
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
