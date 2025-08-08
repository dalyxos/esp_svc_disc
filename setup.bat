@echo off
REM ESP Service Discovery Test Environment Setup Script for Windows
REM ===============================================================

echo 🚀 ESP Service Discovery Test Environment Setup
echo ===============================================

REM Check if Docker is installed
docker --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Docker is not installed. Please install Docker Desktop first.
    exit /b 1
)

docker-compose --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ Docker Compose is not installed. Please install Docker Compose first.
    exit /b 1
)

echo ✅ Docker and Docker Compose are available

REM Create necessary directories
echo 📁 Creating directories...
if not exist "docker\ftp-data\data" mkdir "docker\ftp-data\data"
if not exist "docker\smb-data" mkdir "docker\smb-data"
if not exist "docker\smb-docs" mkdir "docker\smb-docs"
if not exist "docker\ssh-config" mkdir "docker\ssh-config"
if not exist "docker\cups-config" mkdir "docker\cups-config"

echo ✅ Directories created

REM Build containers
echo 🔨 Building Docker containers...
docker-compose build

if %errorlevel% neq 0 (
    echo ❌ Failed to build containers
    exit /b 1
)

echo ✅ Containers built successfully

REM Start services
echo 🚀 Starting services...
docker-compose up -d

if %errorlevel% neq 0 (
    echo ❌ Failed to start services
    exit /b 1
)

echo ⏳ Waiting for services to initialize...
timeout /t 15 /nobreak >nul

echo 🧪 Running initial test...
docker-compose run --rm test-runner python3 /usr/local/bin/test-runner.py

echo.
echo ✅ Setup complete!
echo.
echo Available commands:
echo   docker-compose run --rm test-runner /usr/local/bin/run-tests.sh  - Run tests
echo   docker-compose run --rm test-runner /bin/bash                     - Open test shell
echo   docker-compose run --rm esp-idf /bin/bash                         - Open ESP-IDF shell
echo   docker-compose logs                                                - View logs
echo   docker-compose down                                                - Stop services
echo.
echo Web interface: http://localhost:8080
echo Services are discoverable via mDNS on the 172.20.0.0/24 network
