# ESP Service Discovery Component

An ESP-IDF component for discovering and advertising services on the local network using mDNS (Multicast DNS).

## Features

- **Service Discovery**: Discover services on the local network by service type
- **Service Advertisement**: Advertise your own services to the network
- **Asynchronous Operation**: Non-blocking service discovery with callbacks
- **Easy Integration**: Simple API for quick integration into ESP-IDF projects
- **Multiple Service Types**: Support for various service types (HTTP, FTP, SSH, Printer, etc.)

## Directory Structure

```text
esp_svc_disc/
├── components/
│   └── esp_svc_disc/
│       ├── CMakeLists.txt
│       ├── esp_svc_disc.c
│       └── include/
│           └── esp_svc_disc.h
├── example/
│   ├── CMakeLists.txt
│   ├── sdkconfig.defaults
│   └── main/
│       ├── CMakeLists.txt
│       └── main.c
└── README.md
```

## API Reference

### Core Functions

#### `esp_svc_disc_init()`

Initialize the service discovery component. Must be called before using any other functions.

#### `esp_svc_disc_deinit()`

Deinitialize the component and free resources.

#### `esp_svc_disc_start(const esp_svc_disc_config_t* config)`

Start discovering services with the specified configuration.

#### `esp_svc_disc_stop()`

Stop ongoing service discovery.

### Configuration

```c
typedef struct {
    const char* service_type;           // Service type (e.g., "_http", "_ftp")
    const char* protocol;               // Protocol ("_tcp" or "_udp")
    uint32_t timeout_ms;                // Discovery timeout in milliseconds
    esp_svc_disc_callback_t callback;   // Callback for discovered services
    void* user_data;                    // User data passed to callback
} esp_svc_disc_config_t;
```

### Service Advertisement

#### `esp_svc_disc_set_hostname(const char* hostname)`

Set the hostname for this device.

#### `esp_svc_disc_advertise_service(...)`

Advertise a service on the local network.

#### `esp_svc_disc_remove_service(...)`

Remove an advertised service.

## Usage Example

```c
#include "esp_svc_disc.h"

// Callback function for discovered services
static void service_callback(const char* service_name, 
                           const char* hostname, 
                           uint16_t port,
                           mdns_txt_item_t* txt_records,
                           size_t txt_count,
                           void* user_data)
{
    printf("Found service: %s at %s:%d\\n", service_name, hostname, port);
}

void app_main(void)
{
    // Initialize WiFi connection first...
    
    // Initialize service discovery
    esp_svc_disc_init();
    
    // Set hostname
    esp_svc_disc_set_hostname("my-esp32");
    
    // Configure discovery for HTTP services
    esp_svc_disc_config_t config = {
        .service_type = "_http",
        .protocol = "_tcp",
        .timeout_ms = 3000,
        .callback = service_callback,
        .user_data = NULL
    };
    
    // Start discovery
    esp_svc_disc_start(&config);
    
    // Advertise own service
    mdns_txt_item_t txt_records[] = {
        {"version", "1.0"},
        {"path", "/api"}
    };
    
    esp_svc_disc_advertise_service("My Service", "_http", "_tcp", 80, 
                                  txt_records, 2);
}
```

## Building the Example

### Hardware Testing

1. **Prerequisites**:
   - ESP-IDF v4.4 or later
   - WiFi network for testing
   - ESP-IDF Component Manager (included with ESP-IDF v4.4+)

2. **Configuration**:

   ```bash
   cd example
   idf.py reconfigure  # Install managed components
   idf.py menuconfig
   ```

   Configure your WiFi credentials in the main.c file or through menuconfig.

3. **Build and Flash**:

   ```bash
   idf.py reconfigure  # Install dependencies
   idf.py build
   idf.py flash monitor
   ```

### QEMU Testing

For testing without physical hardware, you can use QEMU emulation:

1. **Prerequisites**:
   - ESP-IDF v4.4 or later
   - QEMU with Xtensa support
   - ESP-IDF Component Manager

2. **Quick Start**:

   **Linux/macOS:**

   ```bash
   chmod +x run-qemu.sh
   ./run-qemu.sh
   ```

   **Windows:**

   ```cmd
   run-qemu.bat
   ```

3. **Manual QEMU Setup**:

   ```bash
   cd example
   idf.py reconfigure  # Install dependencies
   idf.py build
   
   # Create QEMU flash image
   esptool.py --chip esp32 merge_bin -o build/qemu/flash_image.bin \
       --flash_mode dio --flash_freq 40m --flash_size 4MB \
       0x1000 build/bootloader/bootloader.bin \
       0x10000 build/esp_svc_disc_example.bin \
       0x8000 build/partition_table/partition-table.bin
   
   # Run QEMU
   qemu-system-xtensa -machine esp32 -nographic \
       -drive file=build/qemu/flash_image.bin,if=mtd,format=raw \
       -netdev user,id=net0 -device open_eth,netdev=net0
   ```

**Note**: The QEMU version uses Ethernet instead of WiFi for network connectivity.

## Docker Test Environment

This project includes a comprehensive Docker-based test environment that simulates various network services for testing the ESP service discovery component.

### Quick Start

**Linux/macOS:**

```bash
chmod +x setup.sh
./setup.sh
```

**Windows:**

```cmd
setup.bat
```

**Or using Make:**

```bash
make up
make test
```

### Test Environment Services

The Docker environment provides the following discoverable services:

| Service | Type | Port | Description |
|---------|------|------|-------------|
| HTTP Server | `_http._tcp` | 8080 | Nginx web server with API endpoints |
| FTP Server | `_ftp._tcp` | 21 | Pure-FTPd server with test files |
| SSH Server | `_ssh._tcp` | 2222 | OpenSSH server for remote access |
| Print Server | `_ipp._tcp` | 631 | CUPS print server |
| SMB Server | `_smb._tcp` | 445 | Samba file server with shared folders |
| Modbus Server | `_modbus._tcp` | 502 | Modbus TCP server with simulated sensor data |

### Docker Commands

```bash
# Start the test environment
make up

# Run comprehensive tests
make test

# Open test shell for manual testing
make shell

# Open ESP-IDF development shell
make esp-shell

# View service logs
make logs

# Stop all services
make down

# Clean up everything
make clean
```

### Manual Testing

After starting the environment, you can manually test service discovery:

```bash
# Browse all mDNS services
docker-compose run --rm test-runner avahi-browse -a -t

# Test HTTP endpoints
curl http://localhost:8080/
curl http://localhost:8080/api/status

# Run Python test suite
docker-compose run --rm test-runner python3 /usr/local/bin/test-runner.py

# Test Modbus TCP specifically
docker-compose run --rm test-runner python3 -c "
from pymodbus.client.sync import ModbusTcpClient
client = ModbusTcpClient('172.20.0.16', port=502)
if client.connect():
    result = client.read_input_registers(0, 10, unit=1)
    print('Sensor data:', result.registers if not result.isError() else 'Error')
    client.close()
"
```

### Network Configuration

- **Network**: `172.20.0.0/24`
- **Services**: `172.20.0.10-16`
- **Test containers**: `172.20.0.20-21`

All services are configured with proper mDNS advertisements and can be discovered by the ESP service discovery component.

## Example Output

When running the example, you should see output similar to:

```text
I (12345) SVC_DISC_EXAMPLE: ESP Service Discovery Example Starting...
I (12350) SVC_DISC_EXAMPLE: connected to ap SSID:YourWiFi
I (12355) ESP_SVC_DISC: ESP Service Discovery initialized
I (12360) ESP_SVC_DISC: Hostname set to: esp32-svc-disc
I (12365) ESP_SVC_DISC: Service advertised: ESP32 Web Server._http._tcp on port 80
I (12370) SVC_DISC_EXAMPLE: Discovering services: _http._tcp
I (12375) ESP_SVC_DISC: Starting service discovery for _http._tcp
I (15380) SVC_DISC_EXAMPLE: === Service Discovered ===
I (15385) SVC_DISC_EXAMPLE: Service: My Router Admin
I (15390) SVC_DISC_EXAMPLE: Hostname: router.local
I (15395) SVC_DISC_EXAMPLE: Port: 80
I (15400) SVC_DISC_EXAMPLE: ========================
```

## Supported Service Types

The component can discover any mDNS service type. Common examples include:

- `_http._tcp` - HTTP web servers
- `_ftp._tcp` - FTP servers  
- `_ssh._tcp` - SSH servers
- `_printer._tcp` - Network printers
- `_ipp._tcp` - Internet Printing Protocol
- `_smb._tcp` - SMB/CIFS file shares
- `_afpovertcp._tcp` - Apple File Protocol
- `_modbus._tcp` - Modbus TCP industrial protocol

## Dependencies

- ESP-IDF mDNS component (managed via ESP-IDF Component Manager)
- ESP Ethernet component
- ESP Event component
- NVS Flash component

The mDNS component is automatically downloaded and managed by the ESP-IDF Component Manager when you run `idf.py reconfigure`.

## License

This component is provided as-is for educational and development purposes.

## Contributing

Feel free to contribute improvements, bug fixes, or additional features via pull requests.
