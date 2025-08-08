@echo off
REM ESP Service Discovery QEMU Test Script for Windows
REM ==================================================

echo 🚀 ESP Service Discovery QEMU Test
echo ==================================

REM Check if QEMU is available
qemu-system-xtensa --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ❌ QEMU for Xtensa not found. Please install QEMU
    echo    Download from: https://www.qemu.org/download/
    exit /b 1
)

REM Navigate to example directory
cd example

REM Verify we're in the example directory
if not exist "CMakeLists.txt" (
    echo ❌ Error: Example directory not found
    echo    Please ensure you're running this from the project root
    exit /b 1
)

REM Clean previous build
echo 🧹 Cleaning previous build...
idf.py fullclean

REM Install/update managed components
echo 📦 Installing managed components...
idf.py reconfigure

REM Build the project
echo 🔨 Building ESP service discovery example...
idf.py build

if %errorlevel% neq 0 (
    echo ❌ Build failed
    exit /b 1
)

REM Create QEMU flash image
echo 📦 Creating QEMU flash image...
if not exist "build\qemu" mkdir "build\qemu"

REM Merge binary files for QEMU
esptool.py --chip esp32 merge_bin -o build\qemu\flash_image.bin ^
    --flash_mode dio --flash_freq 40m --flash_size 4MB ^
    0x1000 build\bootloader\bootloader.bin ^
    0x10000 build\esp_svc_disc_example.bin ^
    0x8000 build\partition_table\partition-table.bin

if %errorlevel% neq 0 (
    echo ❌ Failed to create QEMU flash image
    exit /b 1
)

echo ✅ QEMU flash image created: build\qemu\flash_image.bin

REM Start QEMU
echo 🚀 Starting QEMU with ESP32 emulation...
echo    Network will use user-mode networking
echo    ESP32 Ethernet will have limited network access
echo.
echo To stop QEMU: Ctrl+A, then X
echo.

REM Run QEMU with user-mode networking (doesn't require admin privileges)
qemu-system-xtensa ^
    -machine esp32 ^
    -nographic ^
    -drive file=build\qemu\flash_image.bin,if=mtd,format=raw ^
    -netdev user,id=net0,hostfwd=tcp::8080-:80 ^
    -device open_eth,netdev=net0 ^
    -serial mon:stdio

echo.
echo QEMU session ended.
