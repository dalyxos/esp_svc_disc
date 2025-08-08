#!/bin/bash

echo "🧪 Running ESP Service Discovery Tests"
echo "======================================"

# Wait for services to be ready
echo "⏳ Waiting for services to start..."
sleep 5

# Run the Python test runner
echo "🐍 Running Python test suite..."
python3 /usr/local/bin/test-runner.py

echo ""
echo "🔍 Additional Manual Tests:"
echo "------------------------"
echo "1. Browse all services:"
echo "   avahi-browse -a -t"
echo ""
echo "2. Resolve specific service:"
echo "   avahi-resolve -n esp-test-webserver.local"
echo ""
echo "3. Test HTTP endpoints:"
echo "   curl http://172.20.0.10:80/"
echo "   curl http://172.20.0.10:80/api/status"
echo ""
echo "4. Check network connectivity:"
echo "   ping 172.20.0.10"
echo "   telnet 172.20.0.11 21"
echo "   telnet 172.20.0.16 502"
echo ""
echo "5. Test Modbus TCP:"
echo "   python3 -c \"from pymodbus.client.sync import ModbusTcpClient; c=ModbusTcpClient('172.20.0.16'); c.connect(); print(c.read_input_registers(0,5).registers); c.close()\""
echo ""
echo "📝 To run these tests again: /usr/local/bin/run-tests.sh"
