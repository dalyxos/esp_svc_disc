#!/usr/bin/env python3

import json
import logging
import time
import threading
from pymodbus.server.sync import StartTcpServer
from pymodbus.device import ModbusDeviceIdentification
from pymodbus.datastore import ModbusSequentialDataBlock, ModbusSlaveContext, ModbusServerContext
from pymodbus.transaction import ModbusRtuFramer, ModbusAsciiFramer
from zeroconf import ServiceInfo, Zeroconf
import socket
import signal
import sys

# Configure logging
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)

class ModbusTcpServerWithMDNS:
    def __init__(self, port=502, device_id=1):
        self.port = port
        self.device_id = device_id
        self.server = None
        self.zeroconf = None
        self.service_info = None
        self.running = False
        
        # Setup signal handlers for graceful shutdown
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
        
    def signal_handler(self, signum, frame):
        logger.info(f"Received signal {signum}, shutting down...")
        self.stop()
        sys.exit(0)
        
    def create_datastore(self):
        """Create a Modbus datastore with some sample data"""
        
        # Initialize data blocks
        # Coils (1-bit, read/write) - 10000 addresses
        coils = ModbusSequentialDataBlock(0, [False] * 100)
        
        # Discrete Inputs (1-bit, read-only) - 10000 addresses  
        discrete_inputs = ModbusSequentialDataBlock(0, [True, False, True, False] * 25)
        
        # Input Registers (16-bit, read-only) - 30000 addresses
        input_registers = ModbusSequentialDataBlock(0, [
            1234,     # Temperature sensor (°C * 10)
            567,      # Humidity sensor (% * 10) 
            890,      # Pressure sensor (kPa)
            2450,     # Voltage (mV)
            1500,     # Current (mA)
            3600,     # Power (W)
            12345,    # Energy counter (Wh)
            98,       # Battery level (%)
            25,       # CPU temperature (°C)
            85        # Signal strength (%)
        ] + [0] * 90)  # Fill remaining with zeros
        
        # Holding Registers (16-bit, read/write) - 40000 addresses
        holding_registers = ModbusSequentialDataBlock(0, [
            1,        # Device enable/disable
            100,      # Setpoint temperature (°C)
            50,       # Setpoint humidity (%)
            1000,     # Alarm threshold
            5,        # Sample rate (seconds)
            0,        # Reset counter
            12,       # Device address
            9600,     # Baud rate
            1,        # Parity (0=none, 1=odd, 2=even)
            1         # Stop bits
        ] + [0] * 90)  # Fill remaining with zeros
        
        # Create slave context
        slave_context = ModbusSlaveContext(
            di=discrete_inputs,
            co=coils, 
            hr=holding_registers,
            ir=input_registers
        )
        
        # Create server context
        context = ModbusServerContext(slaves={self.device_id: slave_context}, single=False)
        return context
        
    def setup_device_identity(self):
        """Setup device identification"""
        identity = ModbusDeviceIdentification()
        identity.VendorName = 'ESP Test Modbus'
        identity.ProductCode = 'ETM'
        identity.VendorUrl = 'http://esp-test-modbus.local'
        identity.ProductName = 'ESP Test Modbus Server'
        identity.ModelName = 'ESP-MODBUS-V1'
        identity.MajorMinorRevision = '1.0.0'
        identity.UserApplicationName = 'ESP Service Discovery Test Server'
        
        return identity
        
    def register_mdns_service(self):
        """Register Modbus service with mDNS"""
        try:
            self.zeroconf = Zeroconf()
            
            # Get local IP address
            hostname = socket.gethostname()
            local_ip = socket.gethostbyname(hostname)
            
            # For Docker, try to get the container IP
            try:
                # This works inside Docker containers
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                    s.connect(("8.8.8.8", 80))
                    local_ip = s.getsockname()[0]
            except:
                pass
                
            logger.info(f"Registering mDNS service at {local_ip}:{self.port}")
            
            # Create service info
            service_type = "_modbus._tcp.local."
            service_name = f"ESP Test Modbus Server.{service_type}"
            
            properties = {
                'version': '1.0.0',
                'device_id': str(self.device_id),
                'protocol': 'modbus-tcp',
                'vendor': 'ESP Test Modbus',
                'model': 'ESP-MODBUS-V1',
                'description': 'ESP Service Discovery Test Modbus Server',
                'registers': 'coils,discrete_inputs,holding_registers,input_registers',
                'functions': '1,2,3,4,5,6,15,16',
                'unit_id': str(self.device_id)
            }
            
            # Convert string properties to bytes
            properties_bytes = {k.encode('utf-8'): v.encode('utf-8') for k, v in properties.items()}
            
            self.service_info = ServiceInfo(
                service_type,
                service_name,
                addresses=[socket.inet_aton(local_ip)],
                port=self.port,
                properties=properties_bytes,
                server=f"esp-test-modbus.local.",
            )
            
            self.zeroconf.register_service(self.service_info)
            logger.info(f"mDNS service registered: {service_name}")
            
        except Exception as e:
            logger.error(f"Failed to register mDNS service: {e}")
            
    def unregister_mdns_service(self):
        """Unregister mDNS service"""
        try:
            if self.zeroconf and self.service_info:
                self.zeroconf.unregister_service(self.service_info)
                self.zeroconf.close()
                logger.info("mDNS service unregistered")
        except Exception as e:
            logger.error(f"Failed to unregister mDNS service: {e}")
            
    def update_sensor_data(self):
        """Simulate changing sensor data"""
        import random
        import math
        
        def update_loop():
            counter = 0
            while self.running:
                try:
                    if self.server and hasattr(self.server, 'context'):
                        slave_context = self.server.context[self.device_id]
                        
                        # Simulate temperature variations (20-30°C)
                        temp = int((25 + 5 * math.sin(counter * 0.1)) * 10)
                        slave_context.setValues(4, 0, [temp])  # Input register 0
                        
                        # Simulate humidity variations (40-80%)
                        humidity = int((60 + 20 * math.sin(counter * 0.05)) * 10)
                        slave_context.setValues(4, 1, [humidity])  # Input register 1
                        
                        # Simulate pressure variations (950-1050 kPa)
                        pressure = int(1000 + 50 * math.sin(counter * 0.03))
                        slave_context.setValues(4, 2, [pressure])  # Input register 2
                        
                        # Random voltage (2200-2800 mV)
                        voltage = random.randint(2200, 2800)
                        slave_context.setValues(4, 3, [voltage])  # Input register 3
                        
                        # Random current (1000-2000 mA)
                        current = random.randint(1000, 2000)
                        slave_context.setValues(4, 4, [current])  # Input register 4
                        
                        # Calculated power
                        power = int((voltage * current) / 1000)  # Watts
                        slave_context.setValues(4, 5, [power])  # Input register 5
                        
                        # Increment energy counter
                        energy = slave_context.getValues(4, 6, 1)[0] + 1
                        if energy > 65535:
                            energy = 0
                        slave_context.setValues(4, 6, [energy])  # Input register 6
                        
                        counter += 1
                        
                except Exception as e:
                    logger.error(f"Error updating sensor data: {e}")
                    
                time.sleep(5)  # Update every 5 seconds
                
        thread = threading.Thread(target=update_loop, daemon=True)
        thread.start()
        
    def start(self):
        """Start the Modbus TCP server"""
        try:
            logger.info(f"Starting Modbus TCP server on port {self.port}")
            
            # Create datastore and device identity
            context = self.create_datastore()
            identity = self.setup_device_identity()
            
            # Register mDNS service
            self.register_mdns_service()
            
            # Start sensor data simulation
            self.running = True
            self.update_sensor_data()
            
            # Start the server (this blocks)
            logger.info("Modbus TCP server is running...")
            StartTcpServer(context, identity=identity, address=("0.0.0.0", self.port))
            
        except Exception as e:
            logger.error(f"Failed to start Modbus server: {e}")
            self.stop()
            
    def stop(self):
        """Stop the server"""
        self.running = False
        self.unregister_mdns_service()
        logger.info("Modbus server stopped")

def main():
    import os
    
    # Get configuration from environment variables
    port = int(os.getenv('MODBUS_PORT', 502))
    device_id = int(os.getenv('MODBUS_DEVICE_ID', 1))
    
    logger.info(f"Starting ESP Test Modbus TCP Server on port {port}, device ID {device_id}")
    
    # Create and start server
    server = ModbusTcpServerWithMDNS(port=port, device_id=device_id)
    server.start()

if __name__ == "__main__":
    main()
