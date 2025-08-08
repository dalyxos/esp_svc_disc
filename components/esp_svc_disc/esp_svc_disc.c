#include "esp_svc_disc.h"
#include "esp_log.h"
#include "esp_eth.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>

static const char *TAG = "ESP_SVC_DISC";

static bool s_mdns_initialized = false;
static TaskHandle_t s_discovery_task = NULL;
static esp_svc_disc_config_t s_current_config = {0};
static bool s_discovery_running = false;

// Event group for synchronization
static EventGroupHandle_t s_discovery_event_group = NULL;
#define DISCOVERY_STOP_BIT BIT0

static void discovery_task(void *pvParameters)
{
    esp_svc_disc_config_t *config = (esp_svc_disc_config_t *)pvParameters;
    
    ESP_LOGI(TAG, "Starting service discovery for %s.%s", config->service_type, config->protocol);
    
    mdns_result_t *results = NULL;
    esp_err_t err = mdns_query_ptr(config->service_type, config->protocol, config->timeout_ms, 20, &results);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS query failed: %s", esp_err_to_name(err));
        goto cleanup;
    }
    
    mdns_result_t *r = results;
    while (r) {
        // Check if we should stop
        EventBits_t bits = xEventGroupWaitBits(s_discovery_event_group, DISCOVERY_STOP_BIT, pdFALSE, pdFALSE, 0);
        if (bits & DISCOVERY_STOP_BIT) {
            ESP_LOGI(TAG, "Discovery stop requested");
            break;
        }
        
        if (r->hostname && config->callback) {
            ESP_LOGI(TAG, "Found service: %s at %s:%d", r->instance_name, r->hostname, r->port);
            config->callback(r->instance_name, r->hostname, r->port, r->txt, r->txt_count, config->user_data);
        }
        r = r->next;
    }
    
cleanup:
    if (results) {
        mdns_query_results_free(results);
    }
    
    s_discovery_running = false;
    s_discovery_task = NULL;
    
    ESP_LOGI(TAG, "Service discovery task completed");
    vTaskDelete(NULL);
}

esp_err_t esp_svc_disc_init(void)
{
    if (s_mdns_initialized) {
        ESP_LOGW(TAG, "mDNS already initialized");
        return ESP_OK;
    }
    
    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    s_discovery_event_group = xEventGroupCreate();
    if (s_discovery_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        mdns_free();
        return ESP_ERR_NO_MEM;
    }
    
    s_mdns_initialized = true;
    ESP_LOGI(TAG, "ESP Service Discovery initialized");
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_deinit(void)
{
    if (!s_mdns_initialized) {
        return ESP_OK;
    }
    
    // Stop any ongoing discovery
    esp_svc_disc_stop();
    
    mdns_free();
    
    if (s_discovery_event_group) {
        vEventGroupDelete(s_discovery_event_group);
        s_discovery_event_group = NULL;
    }
    
    s_mdns_initialized = false;
    ESP_LOGI(TAG, "ESP Service Discovery deinitialized");
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_start(const esp_svc_disc_config_t* config)
{
    if (!s_mdns_initialized) {
        ESP_LOGE(TAG, "Service discovery not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!config || !config->service_type || !config->protocol || !config->callback) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (s_discovery_running) {
        ESP_LOGW(TAG, "Discovery already running, stopping previous discovery");
        esp_svc_disc_stop();
    }
    
    // Copy configuration
    memcpy(&s_current_config, config, sizeof(esp_svc_disc_config_t));
    
    // Clear stop bit
    xEventGroupClearBits(s_discovery_event_group, DISCOVERY_STOP_BIT);
    
    // Create discovery task
    BaseType_t ret = xTaskCreate(discovery_task, "svc_discovery", 4096, &s_current_config, 5, &s_discovery_task);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create discovery task");
        return ESP_ERR_NO_MEM;
    }
    
    s_discovery_running = true;
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_stop(void)
{
    if (!s_discovery_running || !s_discovery_task) {
        return ESP_OK;
    }
    
    // Signal the task to stop
    xEventGroupSetBits(s_discovery_event_group, DISCOVERY_STOP_BIT);
    
    // Wait for task to finish (with timeout)
    int timeout_count = 0;
    while (s_discovery_task != NULL && timeout_count < 50) {
        vTaskDelay(pdMS_TO_TICKS(100));
        timeout_count++;
    }
    
    if (s_discovery_task != NULL) {
        ESP_LOGW(TAG, "Discovery task did not stop gracefully, deleting forcefully");
        vTaskDelete(s_discovery_task);
        s_discovery_task = NULL;
    }
    
    s_discovery_running = false;
    
    ESP_LOGI(TAG, "Service discovery stopped");
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_set_hostname(const char* hostname)
{
    if (!s_mdns_initialized) {
        ESP_LOGE(TAG, "Service discovery not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!hostname) {
        ESP_LOGE(TAG, "Invalid hostname");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = mdns_hostname_set(hostname);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set hostname: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Hostname set to: %s", hostname);
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_advertise_service(const char* instance_name,
                                         const char* service_type,
                                         const char* protocol,
                                         uint16_t port,
                                         mdns_txt_item_t* txt_records,
                                         size_t txt_count)
{
    if (!s_mdns_initialized) {
        ESP_LOGE(TAG, "Service discovery not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!instance_name || !service_type || !protocol) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = mdns_service_add(instance_name, service_type, protocol, port, txt_records, txt_count);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add service: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Service advertised: %s.%s.%s on port %d", instance_name, service_type, protocol, port);
    
    return ESP_OK;
}

esp_err_t esp_svc_disc_remove_service(const char* service_type, const char* protocol)
{
    if (!s_mdns_initialized) {
        ESP_LOGE(TAG, "Service discovery not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (!service_type || !protocol) {
        ESP_LOGE(TAG, "Invalid parameters");
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = mdns_service_remove(service_type, protocol);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to remove service: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Service removed: %s.%s", service_type, protocol);
    
    return ESP_OK;
}
