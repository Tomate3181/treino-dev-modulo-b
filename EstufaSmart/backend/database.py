from sqlalchemy import create_engine
from sqlalchemy.orm import sessionmaker, declarative_base

# SQLite em arquivo local conforme RF-03 da especificação
SQLALCHEMY_DATABASE_URL = "sqlite:///./estufasmart.db"

engine = create_engine(
    SQLALCHEMY_DATABASE_URL, 
    connect_args={"check_same_thread": False}
)

SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

Base = declarative_base()

# Gerenciador de contexto / dependência para sessões do banco de dados
def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
