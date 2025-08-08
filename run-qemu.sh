#!/bin/bash

# ESP Service Discovery QEMU Test Script
# =====================================

set -e

echo "🚀 ESP Service Discovery QEMU Test"
echo "=================================="

# Check if required tools are available
if ! command -v qemu-system-xtensa &> /dev/null; then
    echo "❌ QEMU for Xtensa not found. Please install qemu-system-xtensa"
    echo "   On Ubuntu/Debian: sudo apt-get install qemu-system-misc"
    echo "   On macOS: brew install qemu"
    exit 1
fi

# Navigate to example directory
cd example

# Verify we're in the example directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ Error: Example directory not found"
    echo "   Please ensure you're running this from the project root"
    exit 1
fi

# Clean previous build
echo "🧹 Cleaning previous build..."
idf.py fullclean

# Install/update managed components
echo "📦 Installing managed components..."
idf.py reconfigure

# Build the project
echo "🔨 Building ESP service discovery example..."
idf.py build

# Create QEMU flash image
echo "📦 Creating QEMU flash image..."
mkdir -p build/qemu

# Merge binary files for QEMU
esptool.py --chip esp32 merge_bin -o build/qemu/flash_image.bin \
    --flash_mode dio --flash_freq 40m --flash_size 4MB \
    0x1000 build/bootloader/bootloader.bin \
    0x10000 build/esp_svc_disc_example.bin \
    0x8000 build/partition_table/partition-table.bin

echo "✅ QEMU flash image created: build/qemu/flash_image.bin"

# Start QEMU with networking
echo "🚀 Starting QEMU with ESP32 emulation..."
echo "   Network interface: tap0 (will be created)"
echo "   ESP32 Ethernet will be connected to host network"
echo ""
echo "To stop QEMU: Ctrl+A, then X"
echo "To monitor: Connect to monitor with 'info network'"
echo ""

# Create TAP interface for networking (requires sudo on Linux)
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "Setting up TAP interface (may require sudo)..."
    # Note: This requires additional setup on the host system
    sudo ip tuntap add dev tap0 mode tap 2>/dev/null || echo "TAP interface may already exist"
    sudo ip link set dev tap0 up 2>/dev/null || echo "TAP interface setup may require manual configuration"
    sudo ip addr add 172.20.0.1/24 dev tap0 2>/dev/null || echo "TAP IP may already be configured"
fi

# Run QEMU
qemu-system-xtensa \
    -machine esp32 \
    -nographic \
    -drive file=build/qemu/flash_image.bin,if=mtd,format=raw \
    -netdev tap,id=net0,ifname=tap0,script=no,downscript=no \
    -device open_eth,netdev=net0 \
    -serial mon:stdio \
    -S -gdb tcp::3333

echo ""
echo "QEMU session ended."
