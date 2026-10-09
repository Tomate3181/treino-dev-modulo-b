import os
from fastapi import FastAPI, Depends, HTTPException, Query, status
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from sqlalchemy.orm import Session
from typing import List

from database import engine, Base, get_db
import models
import schemas

# Criação automática das tabelas no SQLite se não existirem (RF-03)
Base.metadata.create_all(bind=engine)

app = FastAPI(
    title="EstufaSmart Backend API",
    description="Servidor central para telemetria IoT do Arduino UNO R4 WiFi e dashboard em tempo real.",
    version="1.0.0"
)

# Configuração de CORS para permitir requisições de dashboards ou clientes externos
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Caminho do template index.html
TEMPLATE_PATH = os.path.join(os.path.dirname(__file__), "templates", "index.html")

# =============================================================================
# RF-05: Interface Web / Dashboard Integrado
# =============================================================================
@app.get("/", response_class=HTMLResponse)
def serve_dashboard():
    """Serve a interface gráfica responsiva em tempo real na rota raiz."""
    if os.path.exists(TEMPLATE_PATH):
        with open(TEMPLATE_PATH, "r", encoding="utf-8") as f:
            return f.read()
    return "<h1>EstufaSmart Dashboard</h1><p>Template index.html não encontrado.</p>"

# =============================================================================
# RF-01 & RF-02: Ingestão de Telemetria com Validação Estrita (Pydantic)
# =============================================================================
@app.post(
    "/api/telemetry", 
    response_model=schemas.TelemetryResponse, 
    status_code=status.HTTP_201_CREATED,
    summary="Recebe e valida o pacote de telemetria do Arduino"
)
def receive_telemetry(payload: schemas.TelemetryCreate, db: Session = Depends(get_db)):
    """
    Ingestão de dados ambientais do Arduino UNO R4 WiFi.
    Valida rigorosamente os dados e salva no banco de dados SQLite.
    """
    db_record = models.Telemetry(
        device_id=payload.device_id,
        placa=payload.placa,
        seq=payload.seq,
        temp=payload.temp,
        umid=payload.umid,
        janela=payload.janela,
        luz_pct=payload.luz_pct,
        estado=payload.estado
    )
    db.add(db_record)
    db.commit()
    db.refresh(db_record)

    print(f"[TELEMETRIA #{db_record.seq}] {db_record.device_id} | "
          f"{db_record.temp:.1f} C | {db_record.umid:.1f}% | "
          f"Janela: {'Aberta' if db_record.janela else 'Fechada'} | Estado: {db_record.estado}")

    return db_record

# Rota alias para compatibilidade com "/api/telemetria"
@app.post("/api/telemetria", response_model=schemas.TelemetryResponse, status_code=status.HTTP_201_CREATED, include_in_schema=False)
def receive_telemetry_alias(payload: schemas.TelemetryCreate, db: Session = Depends(get_db)):
    return receive_telemetry(payload, db)

# =============================================================================
# RF-04: Endpoints de Consulta de Histórico e Estado Atual
# =============================================================================
@app.get(
    "/api/telemetry/latest", 
    response_model=schemas.TelemetryResponse,
    summary="Retorna a telemetria mais recente"
)
def get_latest_telemetry(db: Session = Depends(get_db)):
    """Retorna o registro mais recente gravado no banco de dados."""
    record = db.query(models.Telemetry).order_by(models.Telemetry.id.desc()).first()
    if not record:
        raise HTTPException(
            status_code=status.HTTP_404_NOT_FOUND, 
            detail="Nenhum registro de telemetria encontrado no banco de dados."
        )
    return record

@app.get(
    "/api/telemetry/history", 
    response_model=List[schemas.TelemetryResponse],
    summary="Retorna o histórico de telemetrias ordenado cronologicamente"
)
def get_telemetry_history(limit: int = Query(default=50, ge=1, le=500), db: Session = Depends(get_db)):
    """
    Retorna os últimos N registros de telemetria ordenados cronologicamente (ASC)
    para plotagem precisa no gráfico.
    """
    # Busca os últimos N registros em ordem decrescente e reverte para ordem cronológica
    records = db.query(models.Telemetry).order_by(models.Telemetry.id.desc()).limit(limit).all()
    records.reverse()
    return records

# =============================================================================
# Execução direta via Uvicorn
# =============================================================================
if __name__ == "__main__":
    import uvicorn
    print("=" * 60)
    print("Iniciando Servidor EstufaSmart Backend (FastAPI + SQLite)")
    print("Dashboard: http://localhost:8000")
    print("Swagger Docs: http://localhost:8000/docs")
    print("=" * 60)
    uvicorn.run("main:app", host="0.0.0.0", port=8000, reload=True)
