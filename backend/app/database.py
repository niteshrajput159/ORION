import logging
from sqlalchemy import create_engine
from sqlalchemy.orm import declarative_base, sessionmaker
from app.config import settings

logger = logging.getLogger("oscope.database")

Base = declarative_base()

# Attempt connection to PostgreSQL; fallback to SQLite for lightweight local testing
engine = None
SessionLocal = None

try:
    engine = create_engine(
        settings.database_url,
        pool_pre_ping=True,
        pool_size=10,
        max_overflow=20,
        connect_args={"connect_timeout": 3}
    )
    # Test connection
    with engine.connect() as conn:
        logger.info("Successfully connected to PostgreSQL at %s:%s", settings.DATABASE_HOST, settings.DATABASE_PORT)
    SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)
except Exception as e:
    logger.warning("PostgreSQL unavailable (%s). Falling back to SQLite for local development/testing.", e)
    sqlite_url = "sqlite:///./oscope.db"
    engine = create_engine(
        sqlite_url,
        connect_args={"check_same_thread": False}
    )
    SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

def get_db():
    db = SessionLocal()
    try:
        yield db
    finally:
        db.close()
