from pydantic import BaseModel, Field, field_validator
from datetime import datetime
from typing import Optional, Literal

# Conjunto rigoroso de estados operacionais permitidos (RF-02)
VALID_STATES = ["NORMAL", "ATENÇÃO", "CRÍTICO", "RECONHECIDO", "FALHA_SENSOR"]

class TelemetryCreate(BaseModel):
    device_id: str = Field(..., min_length=1, max_length=50, description="Identificador da estufa", example="ESTUFA_SALA01")
    placa: str = Field(..., min_length=1, max_length=50, description="Modelo da placa", example="UNO_R4_WIFI")
    seq: int = Field(..., ge=1, description="Número sequencial da leitura", example=104)
    temp: float = Field(..., ge=-10.0, le=60.0, description="Temperatura em °C (-10.0 a 60.0)", example=25.4)
    umid: float = Field(..., ge=0.0, le=100.0, description="Umidade relativa do ar (0.0% a 100.0%)", example=60.0)
    janela: bool = Field(..., description="Estado da janela (True=Aberta, False=Fechada)", example=False)
    luz_pct: int = Field(..., ge=0, le=100, description="Luminosidade normalizada (0 a 100%)", example=45)
    estado: str = Field(..., description="Estado operacional da máquina de estados", example="NORMAL")

    @field_validator("estado")
    @classmethod
    def validate_estado(cls, v: str) -> str:
        v_upper = v.strip().upper()
        # Normalização de compatibilidade de acentuação
        if v_upper == "ATENCAO":
            v_upper = "ATENÇÃO"
        elif v_upper == "CRITICO":
            v_upper = "CRÍTICO"

        if v_upper not in VALID_STATES:
            raise ValueError(f"Estado '{v}' inválido. Valores permitidos: {VALID_STATES}")
        return v_upper

class TelemetryResponse(BaseModel):
    id: int
    device_id: str
    placa: str
    seq: int
    temp: float
    umid: float
    janela: bool
    luz_pct: int
    estado: str
    created_at: datetime

    class Config:
        from_attributes = True
