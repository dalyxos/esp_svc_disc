#!/usr/bin/env python3

import asyncio
import socket
import time
import subprocess
import sys
from zeroconf import ServiceBrowser, ServiceListener, Zeroconf
import threading
import requests

class TestServiceListener(ServiceListener):
    def __init__(self):
        self.discovered_services = {}
        self.lock = threading.Lock()

    def remove_service(self, zeroconf, type, name):
        with self.lock:
            if name in self.discovered_services:
                del self.discovered_services[name]
        print(f"Service removed: {name}")

    def add_service(self, zeroconf, type, name):
        info = zeroconf.get_service_info(type, name)
        if info:
            with self.lock:
                self.discovered_services[name] = {
                    'type': type,
                    'name': name,
                    'address': socket.inet_ntoa(info.addresses[0]) if info.addresses else None,
                    'port': info.port,
                    'weight': info.weight,
                    'priority': info.priority,
                    'server': info.server,
                    'properties': {k.decode(): v.decode() if v else '' for k, v in info.properties.items()}
                }
            print(f"Service discovered: {name} at {socket.inet_ntoa(info.addresses[0])}:{info.port}")
            print(f"  Type: {type}")
            print(f"  Properties: {self.discovered_services[name]['properties']}")
            print()

    def get_discovered_services(self):
        with self.lock:
            return dict(self.discovered_services)

def test_network_connectivity():
    """Test basic network connectivity to services"""
    print("=== Testing Network Connectivity ===")
    
    services_to_test = [
        ("esp-test-webserver", 8080, "HTTP"),
        ("172.20.0.11", 21, "FTP"),
        ("172.20.0.12", 2222, "SSH"),
        ("172.20.0.13", 631, "CUPS"),
        ("172.20.0.15", 445, "SMB"),
        ("172.20.0.16", 502, "Modbus TCP")
    ]
    
    results = {}
    
    for host, port, service_name in services_to_test:
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            result = sock.connect_ex((host, port))
            sock.close()
            
            if result == 0:
                print(f"✅ {service_name} ({host}:{port}) - Connection successful")
                results[service_name] = True
            else:
                print(f"❌ {service_name} ({host}:{port}) - Connection failed")
                results[service_name] = False
        except Exception as e:
            print(f"❌ {service_name} ({host}:{port}) - Error: {e}")
            results[service_name] = False
    
    print()
    return results

def test_modbus_service():
    """Test Modbus TCP service specifically"""
    print("=== Testing Modbus TCP Service ===")
    
    try:
        from pymodbus.client.sync import ModbusTcpClient
        
        client = ModbusTcpClient('172.20.0.16', port=502)
        
        if client.connect():
            print("✅ Modbus TCP connection successful")
            
            # Test reading input registers (sensor data)
            try:
                result = client.read_input_registers(0, 10, unit=1)
                if not result.isError():
                    print(f"✅ Read input registers: {result.registers[:5]}...")
                    print(f"   Temperature: {result.registers[0]/10:.1f}°C")
                    print(f"   Humidity: {result.registers[1]/10:.1f}%")
                    print(f"   Pressure: {result.registers[2]} kPa")
                else:
                    print(f"❌ Failed to read input registers: {result}")
            except Exception as e:
                print(f"❌ Error reading input registers: {e}")
            
            # Test reading holding registers (configuration)
            try:
                result = client.read_holding_registers(0, 5, unit=1)
                if not result.isError():
                    print(f"✅ Read holding registers: {result.registers}")
                else:
                    print(f"❌ Failed to read holding registers: {result}")
            except Exception as e:
                print(f"❌ Error reading holding registers: {e}")
            
            # Test writing a holding register
            try:
                result = client.write_register(1, 123, unit=1)  # Write to setpoint register
                if not result.isError():
                    print("✅ Write holding register successful")
                    
                    # Verify the write
                    verify_result = client.read_holding_registers(1, 1, unit=1)
                    if not verify_result.isError() and verify_result.registers[0] == 123:
                        print("✅ Write verification successful")
                    else:
                        print("❌ Write verification failed")
                else:
                    print(f"❌ Failed to write holding register: {result}")
            except Exception as e:
                print(f"❌ Error writing holding register: {e}")
            
            client.close()
        else:
            print("❌ Failed to connect to Modbus server")
            
    except ImportError:
        print("⚠️  pymodbus not available, skipping Modbus tests")
    except Exception as e:
        print(f"❌ Modbus test error: {e}")
    
    print()

def test_http_service():
    """Test HTTP service specifically"""
    print("=== Testing HTTP Service ===")
    
    endpoints = [
        "http://172.20.0.10:80/",
        "http://172.20.0.10:80/api/status",
        "http://172.20.0.10:80/api/info",
        "http://172.20.0.10:80/test"
    ]
    
    for endpoint in endpoints:
        try:
            response = requests.get(endpoint, timeout=5)
            if response.status_code == 200:
                print(f"✅ {endpoint} - Status: {response.status_code}")
                if 'api' in endpoint:
                    try:
                        data = response.json()
                        print(f"   JSON Response: {data}")
                    except:
                        print(f"   Response: {response.text[:100]}...")
            else:
                print(f"❌ {endpoint} - Status: {response.status_code}")
        except Exception as e:
            print(f"❌ {endpoint} - Error: {e}")
    
    print()

def test_mdns_discovery():
    """Test mDNS service discovery"""
    print("=== Testing mDNS Service Discovery ===")
    
    zeroconf = Zeroconf()
    listener = TestServiceListener()
    
    # Service types to discover
    service_types = [
        "_http._tcp.local.",
        "_ftp._tcp.local.",
        "_ssh._tcp.local.",
        "_ipp._tcp.local.",
        "_smb._tcp.local.",
        "_modbus._tcp.local."
    ]
    
    browsers = []
    for service_type in service_types:
        browser = ServiceBrowser(zeroconf, service_type, listener)
        browsers.append(browser)
    
    print("Discovering services for 10 seconds...")
    time.sleep(10)
    
    discovered = listener.get_discovered_services()
    
    print(f"\nDiscovered {len(discovered)} services:")
    for name, info in discovered.items():
        print(f"  {name}")
        print(f"    Type: {info['type']}")
        print(f"    Address: {info['address']}:{info['port']}")
        print(f"    Properties: {info['properties']}")
        print()
    
    # Cleanup
    for browser in browsers:
        browser.cancel()
    zeroconf.close()
    
    return discovered

def test_avahi_browse():
    """Test using avahi-browse command"""
    print("=== Testing Avahi Browse ===")
    
    try:
        result = subprocess.run(['avahi-browse', '-a', '-t', '-r', '-p'], 
                              capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            lines = result.stdout.strip().split('\n')
            services = [line for line in lines if line.startswith('=')]
            
            print(f"Found {len(services)} services via avahi-browse:")
            for service in services[:10]:  # Limit output
                parts = service.split(';')
                if len(parts) >= 4:
                    print(f"  {parts[3]} ({parts[4]})")
            
            if len(services) > 10:
                print(f"  ... and {len(services) - 10} more")
        else:
            print(f"avahi-browse failed: {result.stderr}")
    
    except subprocess.TimeoutExpired:
        print("avahi-browse timed out")
    except Exception as e:
        print(f"Error running avahi-browse: {e}")
    
    print()

def main():
    print("🚀 ESP Service Discovery Test Runner")
    print("=" * 50)
    
    # Test network connectivity
    connectivity_results = test_network_connectivity()
    
    # Test HTTP service
    test_http_service()
    
    # Test mDNS discovery using Python zeroconf
    mdns_results = test_mdns_discovery()
    
    # Test Modbus service
    test_modbus_service()
    
    # Test using avahi-browse
    test_avahi_browse()
    
    # Summary
    print("=== Test Summary ===")
    print(f"Network connectivity tests: {sum(connectivity_results.values())}/{len(connectivity_results)} passed")
    print(f"mDNS services discovered: {len(mdns_results)}")
    
    # Check for expected services
    expected_services = ["_http._tcp", "_ftp._tcp", "_ssh._tcp", "_ipp._tcp", "_smb._tcp", "_modbus._tcp"]
    found_types = set(info['type'] for info in mdns_results.values())
    
    print("\nExpected vs Found service types:")
    for service_type in expected_services:
        service_type_full = f"{service_type}.local."
        if service_type_full in found_types:
            print(f"✅ {service_type}")
        else:
            print(f"❌ {service_type} - Not discovered")
    
    print("\n🏁 Test completed!")

if __name__ == "__main__":
    main()
