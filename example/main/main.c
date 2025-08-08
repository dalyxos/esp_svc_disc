#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_svc_disc.h"

static const char *TAG = "SVC_DISC_EXAMPLE";

// Event group for Ethernet connection
static EventGroupHandle_t s_eth_event_group;
#define ETH_CONNECTED_BIT BIT0
#define ETH_FAIL_BIT      BIT1

static esp_netif_t *eth_netif = NULL;

// Service discovery callback
static void service_discovered_callback(const char* service_name, 
                                      const char* hostname, 
                                      uint16_t port,
                                      mdns_txt_item_t* txt_records,
                                      size_t txt_count,
                                      void* user_data)
{
    ESP_LOGI(TAG, "=== Service Discovered ===");
    ESP_LOGI(TAG, "Service: %s", service_name ? service_name : "Unknown");
    ESP_LOGI(TAG, "Hostname: %s", hostname ? hostname : "Unknown");
    ESP_LOGI(TAG, "Port: %d", port);
    
    if (txt_records && txt_count > 0) {
        ESP_LOGI(TAG, "TXT Records:");
        for (size_t i = 0; i < txt_count; i++) {
            ESP_LOGI(TAG, "  %s = %s", txt_records[i].key, txt_records[i].value ? txt_records[i].value : "");
        }
    }
    ESP_LOGI(TAG, "========================");
}

// Ethernet event handler
static void eth_event_handler(void* arg, esp_event_base_t event_base,
                             int32_t event_id, void* event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        xEventGroupSetBits(s_eth_event_group, ETH_FAIL_BIT);
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

// IP event handler
static void ip_event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    if (event_base == IP_EVENT && event_id == IP_EVENT_ETH_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Ethernet Got IP Address");
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        xEventGroupSetBits(s_eth_event_group, ETH_CONNECTED_BIT);
    }
}

// Initialize Ethernet for QEMU
static esp_err_t ethernet_init(void)
{
    s_eth_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create default ethernet netif
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    eth_netif = esp_netif_new(&cfg);

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &ip_event_handler, NULL));

    // Initialize Ethernet driver for QEMU
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = 1;
    phy_config.reset_gpio_num = -1;

#if CONFIG_IDF_TARGET_ESP32
    // For ESP32 QEMU, use ESP32 EMAC with updated API
    eth_esp32_emac_config_t esp32_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
    esp32_config.smi_mdc_gpio_num = 23;
    esp32_config.smi_mdio_gpio_num = 18;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#else
    // For other targets, use OpenCores Ethernet
    esp_eth_mac_t *mac = esp_eth_mac_new_openeth(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#endif

    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));

    // Attach Ethernet driver to TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

    // Start Ethernet driver
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));

    ESP_LOGI(TAG, "Ethernet initialization finished.");

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_eth_event_group,
            ETH_CONNECTED_BIT | ETH_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(10000));  // 10 second timeout

    if (bits & ETH_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Ethernet connected successfully");
        return ESP_OK;
    } else if (bits & ETH_FAIL_BIT) {
        ESP_LOGI(TAG, "Ethernet connection failed");
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Ethernet connection timeout");
        return ESP_ERR_TIMEOUT;
    }
}

// Task to demonstrate periodic service discovery
static void discovery_task(void *pvParameters)
{
    // Array of service types to discover
    const char* service_types[] = {
        "_http._tcp",
        "_ftp._tcp", 
        "_ssh._tcp",
        "_printer._tcp",
        "_ipp._tcp",
        "_smb._tcp",
        "_afpovertcp._tcp",
        "_modbus._tcp"
    };
    
    int service_count = sizeof(service_types) / sizeof(service_types[0]);
    int current_service = 0;
    
    while (1) {
        // Parse service type and protocol
        char service_type[32];
        char protocol[16];
        const char* full_service = service_types[current_service];
        
        // Find the last dot to separate service type and protocol
        const char* last_dot = strrchr(full_service, '.');
        if (last_dot && last_dot > full_service) {
            // Copy service type (everything before the last dot)
            size_t service_len = last_dot - full_service;
            strncpy(service_type, full_service, service_len);
            service_type[service_len] = '\0';
            
            // Copy protocol (everything after the last dot)
            strcpy(protocol, last_dot);
        } else {
            ESP_LOGE(TAG, "Invalid service format: %s", full_service);
            current_service = (current_service + 1) % service_count;
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }
        
        ESP_LOGI(TAG, "Discovering services: %s%s", service_type, protocol);
        
        esp_svc_disc_config_t config = {
            .service_type = service_type,
            .protocol = protocol,
            .timeout_ms = 3000,
            .callback = service_discovered_callback,
            .user_data = NULL
        };
        
        esp_err_t err = esp_svc_disc_start(&config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to start service discovery: %s", esp_err_to_name(err));
        }
        
        // Wait for discovery to complete
        vTaskDelay(pdMS_TO_TICKS(5000));
        
        // Move to next service type
        current_service = (current_service + 1) % service_count;
        
        // Wait before next discovery cycle
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "ESP Service Discovery Example Starting...");
    
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize Ethernet
    ESP_LOGI(TAG, "Initializing Ethernet...");
    if (ethernet_init() != ESP_OK) {
        ESP_LOGE(TAG, "Ethernet initialization failed");
        return;
    }
    
    // Initialize service discovery
    ESP_LOGI(TAG, "Initializing service discovery...");
    ret = esp_svc_disc_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Service discovery initialization failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // Set hostname for this device
    esp_svc_disc_set_hostname("esp32-svc-disc");
    
    // Advertise a simple HTTP service
    mdns_txt_item_t txt_records[] = {
        {"version", "1.0"},
        {"path", "/"},
        {"description", "ESP32 Service Discovery Example"}
    };
    
    ret = esp_svc_disc_advertise_service("ESP32 Web Server", "_http", "_tcp", 80, 
                                        txt_records, sizeof(txt_records)/sizeof(txt_records[0]));
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "HTTP service advertised successfully");
    } else {
        ESP_LOGW(TAG, "Failed to advertise HTTP service: %s", esp_err_to_name(ret));
    }
    
    // Start discovery task
    ESP_LOGI(TAG, "Starting service discovery task...");
    xTaskCreate(discovery_task, "discovery_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Example setup complete. Discovering services...");
}
