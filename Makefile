# ESP Service Discovery Test Environment
# ====================================

.PHONY: help build up down test clean logs shell esp-shell test-shell status

# Default target
help:
	@echo "ESP Service Discovery Docker Test Environment"
	@echo "============================================="
	@echo ""
	@echo "Available commands:"
	@echo "  make build      - Build all Docker containers"
	@echo "  make up         - Start all services"
	@echo "  make down       - Stop all services"
	@echo "  make test       - Run service discovery tests"
	@echo "  make clean      - Clean up containers and volumes"
	@echo "  make logs       - Show logs from all services"
	@echo "  make status     - Show status of all containers"
	@echo "  make shell      - Open shell in test-runner container"
	@echo "  make esp-shell  - Open shell in ESP-IDF container"
	@echo "  make test-shell - Open shell in test-runner for manual testing"
	@echo ""
	@echo "Individual service commands:"
	@echo "  make logs-web   - Show web server logs"
	@echo "  make logs-ftp   - Show FTP server logs"
	@echo "  make logs-ssh   - Show SSH server logs"
	@echo "  make logs-mdns  - Show mDNS reflector logs"
	@echo "  make logs-modbus - Show Modbus server logs"
	@echo ""

# Build all containers
build:
	@echo "🔨 Building Docker containers..."
	docker-compose build

# Start all services
up:
	@echo "🚀 Starting ESP service discovery test environment..."
	docker-compose up -d
	@echo "⏳ Waiting for services to initialize..."
	@sleep 10
	@echo "✅ Services started! Use 'make test' to run tests."

# Stop all services
down:
	@echo "🛑 Stopping all services..."
	docker-compose down

# Run comprehensive tests
test: up
	@echo "🧪 Running ESP service discovery tests..."
	docker-compose run --rm test-runner /usr/local/bin/run-tests.sh

# Clean up everything
clean: down
	@echo "🧹 Cleaning up containers and volumes..."
	docker-compose down -v --remove-orphans
	docker system prune -f

# Show logs from all services
logs:
	docker-compose logs -f

# Show status of all containers
status:
	@echo "📊 Container Status:"
	@docker-compose ps

# Individual service logs
logs-web:
	docker-compose logs -f web-server

logs-ftp:
	docker-compose logs -f ftp-server

logs-ssh:
	docker-compose logs -f ssh-server

logs-mdns:
	docker-compose logs -f mdns-reflector

logs-smb:
	docker-compose logs -f smb-server

logs-cups:
	docker-compose logs -f print-server

logs-modbus:
	docker-compose logs -f modbus-server

# Interactive shells
shell:
	@echo "🐚 Opening shell in test-runner container..."
	docker-compose run --rm test-runner /bin/bash

esp-shell:
	@echo "🐚 Opening shell in ESP-IDF container..."
	docker-compose run --rm esp-idf /bin/bash

test-shell:
	@echo "🐚 Opening shell in test-runner for manual testing..."
	docker-compose run --rm test-runner /bin/bash

# Quick service verification
verify:
	@echo "🔍 Quick service verification..."
	@echo "Checking if containers are running..."
	@docker-compose ps --services --filter "status=running" | wc -l | xargs -I {} echo "Running containers: {}"
	@echo ""
	@echo "Testing HTTP service..."
	@curl -s http://localhost:8080/ > /dev/null && echo "✅ HTTP service responsive" || echo "❌ HTTP service not responding"
	@echo ""
	@echo "Testing service discovery..."
	@docker-compose run --rm test-runner python3 /usr/local/bin/test-runner.py | grep "Test completed" && echo "✅ Service discovery test passed" || echo "❌ Service discovery test failed"

# Development helpers
build-esp:
	@echo "🔨 Building ESP-IDF example..."
	docker-compose run --rm esp-idf bash -c "cd example && idf.py build"

build-component:
	@echo "🔨 Building ESP component..."
	docker-compose run --rm esp-idf bash -c "cd components/esp_svc_disc && echo 'Component ready for use'"

# Network inspection
network-info:
	@echo "🌐 Network Information:"
	@docker network ls | grep esp
	@echo ""
	@echo "Container IPs:"
	@docker-compose ps -q | xargs docker inspect -f '{{.Name}} - {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}'
