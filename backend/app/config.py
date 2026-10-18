import os
from pydantic_settings import BaseSettings

class Settings(BaseSettings):
    DATABASE_HOST: str = os.getenv("DATABASE_HOST", "localhost")
    DATABASE_PORT: int = int(os.getenv("DATABASE_PORT", "5432"))
    DATABASE_NAME: str = os.getenv("DATABASE_NAME", "orion")
    DATABASE_USER: str = os.getenv("DATABASE_USER", "orion")
    DATABASE_PASSWORD: str = os.getenv("DATABASE_PASSWORD", "orion_password")
    
    BACKEND_HOST: str = os.getenv("BACKEND_HOST", "0.0.0.0")
    BACKEND_PORT: int = int(os.getenv("BACKEND_PORT", "8000"))
    ENVIRONMENT: str = os.getenv("ENVIRONMENT", "development")
    
    OSCOPE_IPC_SOCKET: str = os.getenv("ORION_IPC_SOCKET", os.getenv("OSCOPE_IPC_SOCKET", "/tmp/oscope_core.sock"))
    OSCOPE_CONFIG_PATH: str = os.getenv("ORION_CONFIG_PATH", os.getenv("OSCOPE_CONFIG_PATH", "config/config.yaml"))
    OSCOPE_CORE_BIN_DIR: str = os.getenv("ORION_CORE_BIN_DIR", os.getenv("OSCOPE_CORE_BIN_DIR", "../core/bin"))
    TEST_WORKLOADS_DIR: str = os.getenv("TEST_WORKLOADS_DIR", "../test_workloads")

    @property
    def database_url(self) -> str:
        # Default to PostgreSQL
        return f"postgresql://{self.DATABASE_USER}:{self.DATABASE_PASSWORD}@{self.DATABASE_HOST}:{self.DATABASE_PORT}/{self.DATABASE_NAME}"

settings = Settings()
