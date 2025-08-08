#!/bin/bash

echo "Starting ESP Test Modbus TCP Server..."

# Start the Modbus server
exec python3 /app/modbus_server.py
