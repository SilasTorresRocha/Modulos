Pela API e possível consumir dados com:


# Pegar os 50 últimos dados de telemetria (Padrão):
curl -X GET "https://apisilas.ddns.net/api/v1/telemetria?chave_api=SUA_CHAVE_AQUI" 

# Pegar com limite específico (Ex: os últimos 10 dados):
curl -X GET "https://apisilas.ddns.net/api/v1/telemetria?chave_api=SUA_CHAVE_AQUI&limite=10" 

# Enviar um Comando externamente:
curl -X POST "https://apisilas.ddns.net/api/v1/comandos/enviar_externo?chave_api=SUA_CHAVE_AQUI" -d "comando=ON"