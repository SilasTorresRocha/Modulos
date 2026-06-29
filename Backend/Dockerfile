FROM python:3.11-slim

WORKDIR /app

# Instalar dependências primeiro para aproveitar o cache do Docker
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

# Copiar todo o código
COPY . .

# Expor a porta
EXPOSE 8000

# Comando para rodar a aplicação via Uvicorn
CMD ["uvicorn", "main:app", "--host", "0.0.0.0", "--port", "8000"]
