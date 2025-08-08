#include "unity.h"
#include "esp_svc_disc.h"
#include "esp_log.h"

static const char *TAG = "ESP_SVC_DISC_TEST";

// Test callback function
static int callback_count = 0;
static void test_callback(const char* service_name, 
                         const char* hostname, 
                         uint16_t port,
                         mdns_txt_item_t* txt_records,
                         size_t txt_count,
                         void* user_data)
{
    callback_count++;
    ESP_LOGI(TAG, "Test callback called for service: %s", service_name ? service_name : "Unknown");
}

TEST_CASE("esp_svc_disc_init_deinit", "[esp_svc_disc]")
{
    // Test initialization
    esp_err_t ret = esp_svc_disc_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test double initialization (should be OK)
    ret = esp_svc_disc_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test deinitialization
    ret = esp_svc_disc_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test double deinitialization (should be OK)
    ret = esp_svc_disc_deinit();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
}

TEST_CASE("esp_svc_disc_hostname", "[esp_svc_disc]")
{
    // Initialize first
    esp_err_t ret = esp_svc_disc_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test setting hostname
    ret = esp_svc_disc_set_hostname("test-esp32");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test setting NULL hostname (should fail)
    ret = esp_svc_disc_set_hostname(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Cleanup
    esp_svc_disc_deinit();
}

TEST_CASE("esp_svc_disc_config_validation", "[esp_svc_disc]")
{
    // Initialize first
    esp_err_t ret = esp_svc_disc_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test with NULL config
    ret = esp_svc_disc_start(NULL);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid config (NULL service_type)
    esp_svc_disc_config_t config = {
        .service_type = NULL,
        .protocol = "_tcp",
        .timeout_ms = 3000,
        .callback = test_callback,
        .user_data = NULL
    };
    ret = esp_svc_disc_start(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid config (NULL protocol)
    config.service_type = "_http";
    config.protocol = NULL;
    ret = esp_svc_disc_start(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Test with invalid config (NULL callback)
    config.protocol = "_tcp";
    config.callback = NULL;
    ret = esp_svc_disc_start(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Cleanup
    esp_svc_disc_deinit();
}

TEST_CASE("esp_svc_disc_service_advertisement", "[esp_svc_disc]")
{
    // Initialize first
    esp_err_t ret = esp_svc_disc_init();
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test advertising a service
    mdns_txt_item_t txt_records[] = {
        {"version", "1.0"},
        {"test", "true"}
    };
    
    ret = esp_svc_disc_advertise_service("Test Service", "_http", "_tcp", 8080, 
                                        txt_records, 2);
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test removing the service
    ret = esp_svc_disc_remove_service("_http", "_tcp");
    TEST_ASSERT_EQUAL(ESP_OK, ret);
    
    // Test with invalid parameters
    ret = esp_svc_disc_advertise_service(NULL, "_http", "_tcp", 8080, NULL, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    ret = esp_svc_disc_advertise_service("Test", NULL, "_tcp", 8080, NULL, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    ret = esp_svc_disc_advertise_service("Test", "_http", NULL, 8080, NULL, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_ARG, ret);
    
    // Cleanup
    esp_svc_disc_deinit();
}

TEST_CASE("esp_svc_disc_without_init", "[esp_svc_disc]")
{
    // Test functions without initialization (should fail)
    esp_err_t ret = esp_svc_disc_set_hostname("test");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    esp_svc_disc_config_t config = {
        .service_type = "_http",
        .protocol = "_tcp",
        .timeout_ms = 3000,
        .callback = test_callback,
        .user_data = NULL
    };
    ret = esp_svc_disc_start(&config);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    ret = esp_svc_disc_advertise_service("Test", "_http", "_tcp", 8080, NULL, 0);
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
    
    ret = esp_svc_disc_remove_service("_http", "_tcp");
    TEST_ASSERT_EQUAL(ESP_ERR_INVALID_STATE, ret);
}
