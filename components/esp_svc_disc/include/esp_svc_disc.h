#pragma once

#include "esp_err.h"
#include "mdns.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Service discovery callback function type
 * 
 * @param service_name Name of the discovered service
 * @param hostname Hostname of the service
 * @param port Port number of the service
 * @param txt_records TXT records associated with the service
 * @param txt_count Number of TXT records
 * @param user_data User data passed to the discovery function
 */
typedef void (*esp_svc_disc_callback_t)(const char* service_name, 
                                        const char* hostname, 
                                        uint16_t port,
                                        mdns_txt_item_t* txt_records,
                                        size_t txt_count,
                                        void* user_data);

/**
 * @brief Configuration structure for service discovery
 */
typedef struct {
    const char* service_type;           ///< Service type (e.g., "_http", "_ftp")
    const char* protocol;               ///< Protocol ("_tcp" or "_udp")
    uint32_t timeout_ms;                ///< Discovery timeout in milliseconds
    esp_svc_disc_callback_t callback;   ///< Callback function for discovered services
    void* user_data;                    ///< User data to pass to callback
} esp_svc_disc_config_t;

/**
 * @brief Initialize the service discovery component
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_init(void);

/**
 * @brief Deinitialize the service discovery component
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_deinit(void);

/**
 * @brief Start discovering services on the local network
 * 
 * @param config Configuration for service discovery
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_start(const esp_svc_disc_config_t* config);

/**
 * @brief Stop ongoing service discovery
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_stop(void);

/**
 * @brief Set the hostname for this device (for mDNS advertising)
 * 
 * @param hostname Hostname to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_set_hostname(const char* hostname);

/**
 * @brief Advertise a service on the local network
 * 
 * @param instance_name Instance name of the service
 * @param service_type Service type (e.g., "_http")
 * @param protocol Protocol ("_tcp" or "_udp")
 * @param port Port number
 * @param txt_records Array of TXT records (can be NULL)
 * @param txt_count Number of TXT records
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_advertise_service(const char* instance_name,
                                         const char* service_type,
                                         const char* protocol,
                                         uint16_t port,
                                         mdns_txt_item_t* txt_records,
                                         size_t txt_count);

/**
 * @brief Remove an advertised service
 * 
 * @param service_type Service type
 * @param protocol Protocol
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t esp_svc_disc_remove_service(const char* service_type, const char* protocol);

#ifdef __cplusplus
}
#endif
