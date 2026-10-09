import sys
from pydantic import ValidationError
from database import SessionLocal, engine, Base
import models
import schemas
import main

def run_tests():
    print("Iniciando testes funcionais diretos do Backend FastAPI...")
    
    # Recria tabelas de teste
    Base.metadata.create_all(bind=engine)
    db = SessionLocal()

    try:
        # 1. Teste de Validação Pydantic - Telemetria Válida
        valid_data = {
            "device_id": "ESTUFA_SALA01",
            "placa": "UNO_R4_WIFI",
            "seq": 101,
            "temp": 24.5,
            "umid": 62.0,
            "janela": False,
            "luz_pct": 50,
            "estado": "NORMAL"
        }
        item = schemas.TelemetryCreate(**valid_data)
        assert item.seq == 101
        print("[OK] RF-02: TelemetryCreate validou dados corretos.")

        # 2. Ingestão no banco via rota receive_telemetry
        res = main.receive_telemetry(item, db)
        assert res.id is not None
        assert res.seq == 101
        assert res.estado == "NORMAL"
        print("[OK] RF-01 & RF-03: Rota POST /api/telemetry persistiu no SQLite estufasmart.db com sucesso.")

        # 3. Teste de Validação Pydantic - Temperatura Inválida (> 60°C)
        try:
            inv = valid_data.copy()
            inv["temp"] = 85.0
            schemas.TelemetryCreate(**inv)
            assert False, "Deveria ter falhado para temp > 60°C"
        except ValidationError:
            print("[OK] RF-02: Temperatura invalida (> 60 C) rejeitada pelo validador Pydantic.")

        # 4. Teste de Validação Pydantic - Luminosidade Inválida (> 100%)
        try:
            inv = valid_data.copy()
            inv["luz_pct"] = 150
            schemas.TelemetryCreate(**inv)
            assert False, "Deveria ter falhado para luz_pct > 100"
        except ValidationError:
            print("[OK] RF-02: Luminosidade invalida (> 100%) rejeitada pelo validador Pydantic.")

        # 5. Teste de Validação Pydantic - Estado Inválido
        try:
            inv = valid_data.copy()
            inv["estado"] = "ESTADO_INEXISTENTE"
            schemas.TelemetryCreate(**inv)
            assert False, "Deveria ter falhado para estado invalido"
        except ValidationError:
            print("[OK] RF-02: Estado invalido rejeitado pelo validador Pydantic.")

        # 6. Inserir múltiplos registros e testar GET /api/telemetry/latest e history
        for seq, t, u, st in [
            (102, 26.2, 59.0, "ATENÇÃO"), 
            (103, 29.8, 54.0, "CRÍTICO"), 
            (104, 25.1, 58.5, "RECONHECIDO")
        ]:
            item_seq = schemas.TelemetryCreate(
                device_id="ESTUFA_SALA01",
                placa="UNO_R4_WIFI",
                seq=seq,
                temp=t,
                umid=u,
                janela=False,
                luz_pct=45,
                estado=st
            )
            main.receive_telemetry(item_seq, db)

        latest = main.get_latest_telemetry(db)
        assert latest.seq == 104
        assert latest.estado == "RECONHECIDO"
        print("[OK] RF-04: get_latest_telemetry retornou o registro mais recente (seq #104).")

        history = main.get_telemetry_history(limit=10, db=db)
        assert len(history) >= 4
        assert history[0].seq <= history[-1].seq
        print("[OK] RF-04: get_telemetry_history retornou historico em ordem cronologica.")

        # 7. Teste de GET / (Dashboard HTML)
        html = main.serve_dashboard()
        assert "EstufaSmart IoT" in html
        assert "Chart.js" in html or "telemetryChart" in html
        print("[OK] RF-05: serve_dashboard retornou template HTML com Chart.js e metricas.")

        print("\nTODOS OS REQUISITOS DO BACKEND FASTAPI FORAM VALIDADOS COM 100% DE SUCESSO!")
    finally:
        db.close()

if __name__ == "__main__":
    run_tests()
