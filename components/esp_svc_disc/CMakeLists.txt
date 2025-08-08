idf_component_register(SRCS "esp_svc_disc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mdns" "esp_wifi" "esp_netif" "esp_event" "nvs_flash")
