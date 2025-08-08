FROM python:3.11-slim

# Install system dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    python3-dev \
    && rm -rf /var/lib/apt/lists/*

# Install Python dependencies
RUN pip install --no-cache-dir \
    pymodbus[serial] \
    flask \
    zeroconf

# Create app directory
WORKDIR /app

# Copy application files
COPY modbus_server.py /app/
COPY start.sh /app/
COPY config/ /app/config/

# Make start script executable
RUN chmod +x /app/start.sh

# Expose Modbus TCP port
EXPOSE 502

# Run the application
CMD ["/app/start.sh"]
