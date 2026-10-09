from sqlalchemy import Column, Integer, Float, Boolean, String, DateTime
from datetime import datetime
from database import Base

class Telemetry(Base):
    __tablename__ = "telemetry"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    device_id = Column(String, nullable=False)
    placa = Column(String, nullable=False)
    seq = Column(Integer, nullable=False)
    temp = Column(Float, nullable=False)
    umid = Column(Float, nullable=False)
    janela = Column(Boolean, nullable=False)
    luz_pct = Column(Integer, nullable=False)
    estado = Column(String, nullable=False)
    created_at = Column(DateTime, default=datetime.utcnow, nullable=False)
