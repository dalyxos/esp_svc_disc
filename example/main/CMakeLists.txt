idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "esp_svc_disc" "esp_wifi" "esp_netif" "esp_event" "nvs_flash")
