#!/bin/bash

# Source ESP-IDF environment
source /opt/esp/idf/export.sh

echo "ESP-IDF Environment Ready"
echo "IDF_PATH: $IDF_PATH"
echo "Available commands:"
echo "  idf.py - ESP-IDF build tool"
echo "  avahi-browse - Browse mDNS services"
echo "  dig - DNS lookup tool"
echo "  curl - HTTP client"
echo ""
echo "To build the example:"
echo "  cd example && idf.py build"
echo ""
echo "To browse mDNS services:"
echo "  avahi-browse -a -t"
echo ""

# Execute the passed command
exec "$@"
