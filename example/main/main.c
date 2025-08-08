#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_svc_disc.h"

// WiFi credentials - modify these for your network
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASS "YOUR_WIFI_PASSWORD"

static const char *TAG = "SVC_DISC_EXAMPLE";

// Event group for WiFi connection
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static int s_retry_num = 0;
static const int WIFI_MAXIMUM_RETRY = 5;

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

// WiFi event handler
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initialize WiFi in station mode
static esp_err_t wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    // Wait for connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s", WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
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
    
    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    if (wifi_init_sta() != ESP_OK) {
        ESP_LOGE(TAG, "WiFi initialization failed");
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
