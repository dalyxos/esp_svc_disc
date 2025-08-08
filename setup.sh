#!/bin/bash

# ESP Service Discovery Test Environment Setup Script
# ==================================================

set -e

echo "🚀 ESP Service Discovery Test Environment Setup"
echo "==============================================="

# Check if Docker and Docker Compose are installed
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed. Please install Docker first."
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose is not installed. Please install Docker Compose first."
    exit 1
fi

echo "✅ Docker and Docker Compose are available"

# Create necessary directories
echo "📁 Creating directories..."
mkdir -p docker/ftp-data/data
mkdir -p docker/smb-data
mkdir -p docker/smb-docs
mkdir -p docker/ssh-config
mkdir -p docker/cups-config

echo "✅ Directories created"

# Set permissions for SSH
echo "🔐 Setting up SSH configuration..."
if [ ! -f docker/ssh-config/ssh_host_rsa_key ]; then
    ssh-keygen -t rsa -f docker/ssh-config/ssh_host_rsa_key -N ""
fi

echo "✅ SSH keys generated"

# Build containers
echo "🔨 Building Docker containers..."
docker-compose build

echo "✅ Containers built successfully"

# Start services
echo "🚀 Starting services..."
docker-compose up -d

echo "⏳ Waiting for services to initialize..."
sleep 15

echo "🧪 Running initial test..."
docker-compose run --rm test-runner python3 /usr/local/bin/test-runner.py

echo ""
echo "✅ Setup complete!"
echo ""
echo "Available commands:"
echo "  make test       - Run comprehensive tests"
echo "  make shell      - Open test shell"
echo "  make esp-shell  - Open ESP-IDF shell"
echo "  make logs       - View service logs"
echo "  make down       - Stop all services"
echo "  make clean      - Clean up everything"
echo ""
echo "Web interface: http://localhost:8080"
echo "Services are discoverable via mDNS on the 172.20.0.0/24 network"
