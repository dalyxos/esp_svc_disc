idf_component_register(SRCS "esp_svc_disc.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "mdns" "esp_eth" "esp_netif" "esp_event" "nvs_flash")
